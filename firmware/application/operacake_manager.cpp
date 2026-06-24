/*
 * Copyright (C) 2026 Mayhem firmware contributors
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "operacake_manager.hpp"

#include "app_settings.hpp"
#include "convert.hpp"       // parse_int
#include "file_reader.hpp"  // split_string
#include "portapack.hpp"

#include <string>

namespace operacake {

/* ---- PCA9557 register map (from HackRF operacake.c) ---------------------- */
static constexpr uint8_t REG_INPUT = 0x00;
static constexpr uint8_t REG_OUTPUT = 0x01;
static constexpr uint8_t REG_POLARITY = 0x02;
static constexpr uint8_t REG_CONFIG = 0x03;

/* ---- OUTPUT-register bit layout ----------------------------------------- */
/* bit7 OE: 1 = GPIO disabled => use register/I2C control (a.k.a GPIO_DISABLE) */
static constexpr uint8_t OE_REGISTER_CONTROL = 1 << 7;  // 0x80
static constexpr uint8_t SAMESIDE = 1 << 2;             // 0x04
static constexpr uint8_t CROSSOVER = 0;                 // 0x00
/* Replicates HackRF's EN_LEDS = (LEDEN2(1) | LEDEN2(0)) == 0x02 verbatim. */
static constexpr uint8_t EN_LEDS = 1 << 1;  // 0x02

static constexpr uint8_t PORT_A1 = 0;                    // 0x00
static constexpr uint8_t PORT_A2 = 1 << 5;               // 0x20
static constexpr uint8_t PORT_A3 = 1 << 6;               // 0x40
static constexpr uint8_t PORT_A4 = (1 << 5) | (1 << 6);  // 0x60
static constexpr uint8_t PORT_B1 = 0;                    // 0x00
static constexpr uint8_t PORT_B2 = 1 << 3;               // 0x08
static constexpr uint8_t PORT_B3 = 1 << 4;               // 0x10
static constexpr uint8_t PORT_B4 = (1 << 3) | (1 << 4);  // 0x18

static constexpr uint8_t CONFIG_ALL_OUTPUT = 0x00;
static constexpr uint8_t DEFAULT_OUTPUT =
    OE_REGISTER_CONTROL | SAMESIDE | PORT_A1 | PORT_B1 | EN_LEDS;  // 0x86

static constexpr uint8_t I2C_BASE_ADDRESS = 0x18;
static constexpr uint8_t INVALID_RANGE = 0xFF;
static constexpr systime_t I2C_TIMEOUT = 50;  // matches codec-init convention

static constexpr char SETTINGS_STORE[] = "operacake";

/* Translate a port index (PA1..PB4) to its OUTPUT-register select bits. */
static uint8_t port_to_pins(uint8_t port) {
    switch (port) {
        case PA1:
            return PORT_A1;
        case PA2:
            return PORT_A2;
        case PA3:
            return PORT_A3;
        case PA4:
            return PORT_A4;
        case PB1:
            return PORT_B1;
        case PB2:
            return PORT_B2;
        case PB3:
            return PORT_B3;
        case PB4:
            return PORT_B4;
        default:
            return 0xFF;
    }
}

OperaCakeManager& OperaCakeManager::instance() {
    static OperaCakeManager manager;
    return manager;
}

bool OperaCakeManager::write_reg(uint8_t board, uint8_t reg, uint8_t value) {
    const uint8_t data[2] = {reg, value};
    return portapack::i2c0.transmit(I2C_BASE_ADDRESS + board, data, 2, I2C_TIMEOUT);
}

bool OperaCakeManager::read_reg(uint8_t board, uint8_t reg, uint8_t& out) {
    const uint8_t cmd[1] = {reg};
    if (!portapack::i2c0.transmit(I2C_BASE_ADDRESS + board, cmd, 1, I2C_TIMEOUT))
        return false;
    return portapack::i2c0.receive(I2C_BASE_ADDRESS + board, &out, 1, I2C_TIMEOUT);
}

bool OperaCakeManager::detect_boards() {
    present_boards_.clear();

    for (uint8_t addr = 0; addr < max_boards; addr++) {
        /* Initialise to the safe default and put the expander in register
         * (I2C) control mode. A board that ACKs and reads CONFIG back as
         * all-outputs is present. */
        bool ok = write_reg(addr, REG_OUTPUT, DEFAULT_OUTPUT);
        ok = write_reg(addr, REG_CONFIG, CONFIG_ALL_OUTPUT) && ok;

        uint8_t cfg = 0xFF;
        bool present = ok && read_reg(addr, REG_CONFIG, cfg) &&
                       (cfg == CONFIG_ALL_OUTPUT);

        state_[addr].present = present;
        if (present) {
            present_boards_.push_back(addr);
            /* A freshly (re)detected board is back at its hardware default. */
            state_[addr].PA = PA1;
            state_[addr].PB = PB1;
        }
    }

    current_range_ = INVALID_RANGE;
    recompute_frequency_flag();
    return !present_boards_.empty();
}

bool OperaCakeManager::board_present(uint8_t board) const {
    return board < max_boards && state_[board].present;
}

uint8_t OperaCakeManager::activate_ports(uint8_t board, uint8_t PA, uint8_t PB) {
    if (board >= max_boards)
        return 1;

    /* Validate range and the opposite-side rule (mirrors operacake.c). */
    if ((PA > PB4) || (PB > PB4))
        return 1;
    if (((PA <= PA4) && (PB <= PA4)) || ((PA > PA4) && (PB > PA4)))
        return 1;

    const uint8_t side = (PA > PA4) ? CROSSOVER : SAMESIDE;
    const uint8_t pa_bits = port_to_pins(PA);
    const uint8_t pb_bits = port_to_pins(PB);
    const uint8_t reg = OE_REGISTER_CONTROL | side | pa_bits | pb_bits | EN_LEDS;

    return write_reg(board, REG_OUTPUT, reg) ? 0 : 1;
}

Mode OperaCakeManager::mode(uint8_t board) const {
    if (board >= max_boards)
        return Mode::Manual;
    return state_[board].mode;
}

void OperaCakeManager::set_mode(uint8_t board, Mode mode) {
    if (board >= max_boards)
        return;

    state_[board].mode = mode;
    current_range_ = INVALID_RANGE;
    recompute_frequency_flag();

    /* Manual mode: re-assert the stored manual ports immediately.
     * Frequency mode: leave the ports as-is; the next retune (or an explicit
     * apply-for-frequency) will set them via on_frequency_changed(). */
    if (mode == Mode::Manual && state_[board].present)
        activate_ports(board, state_[board].PA, state_[board].PB);
}

void OperaCakeManager::set_manual_ports(uint8_t board, uint8_t PA, uint8_t PB) {
    if (board >= max_boards)
        return;

    state_[board].PA = PA;
    state_[board].PB = PB;

    if (state_[board].mode == Mode::Manual && state_[board].present)
        activate_ports(board, PA, PB);
}

uint8_t OperaCakeManager::manual_PA(uint8_t board) const {
    return board < max_boards ? state_[board].PA : static_cast<uint8_t>(PA1);
}

uint8_t OperaCakeManager::manual_PB(uint8_t board) const {
    return board < max_boards ? state_[board].PB : static_cast<uint8_t>(PB1);
}

void OperaCakeManager::clear_ranges() {
    ranges_.clear();
    current_range_ = INVALID_RANGE;
}

uint8_t OperaCakeManager::add_range(uint16_t min_mhz, uint16_t max_mhz, uint8_t portA) {
    if (ranges_.size() >= max_ranges)
        return 1;
    ranges_.push_back({min_mhz, max_mhz, static_cast<uint8_t>(portA & 0x07)});
    current_range_ = INVALID_RANGE;
    return 0;
}

void OperaCakeManager::set_ranges(const std::vector<FreqRange>& ranges) {
    ranges_.clear();
    for (const auto& r : ranges) {
        if (ranges_.size() >= max_ranges)
            break;
        ranges_.push_back(r);
    }
    current_range_ = INVALID_RANGE;
}

void OperaCakeManager::recompute_frequency_flag() {
    any_frequency_mode_ = false;
    for (uint8_t addr : present_boards_) {
        if (state_[addr].mode == Mode::Frequency) {
            any_frequency_mode_ = true;
            return;
        }
    }
}

void OperaCakeManager::on_frequency_changed(uint64_t freq_hz) {
    /* Cheap early-out: this runs on every retune, including fast sweeps. */
    if (!any_frequency_mode_ || ranges_.empty())
        return;

    const uint32_t freq_mhz = static_cast<uint32_t>(freq_hz / 1000000u);

    /* Ranges are in priority order; the last range is the catch-all default. */
    size_t range = 0;
    for (; range < ranges_.size(); range++) {
        if (freq_mhz >= ranges_[range].min_mhz && freq_mhz <= ranges_[range].max_mhz)
            break;
    }
    if (range == ranges_.size())
        range = ranges_.size() - 1;

    /* Only touch I2C when the active band actually changes. */
    if (range == current_range_)
        return;

    const uint8_t portA = ranges_[range].portA & 0x07;
    const uint8_t portB = static_cast<uint8_t>((portA + 4) % 8);  // B mirrors A

    for (uint8_t addr : present_boards_) {
        if (state_[addr].mode == Mode::Frequency)
            activate_ports(addr, portA, portB);
    }

    current_range_ = static_cast<uint8_t>(range);
}

/* ---- Persistence -------------------------------------------------------- */

/* The whole config is packed into one comma-separated "cfg" value, which keeps
 * the code (and flash footprint) far smaller than binding ~50 named scalars.
 * Field order: for each of 8 boards mode,PA,PB; then the range count; then
 * min,max,portA for each of the 8 range slots. */

void OperaCakeManager::save() const {
    std::string s;
    for (size_t i = 0; i < max_boards; i++) {
        s += std::to_string(static_cast<unsigned>(state_[i].mode)) + ',';
        s += std::to_string(state_[i].PA) + ',';
        s += std::to_string(state_[i].PB) + ',';
    }
    s += std::to_string(ranges_.size()) + ',';
    for (size_t i = 0; i < max_ranges; i++) {
        const FreqRange r = (i < ranges_.size()) ? ranges_[i] : FreqRange{0, 0, 0};
        s += std::to_string(r.min_mhz) + ',';
        s += std::to_string(r.max_mhz) + ',';
        s += std::to_string(r.portA) + ',';
    }

    SettingBindings bindings;
    bindings.emplace_back(std::string_view{"cfg"}, &s);
    save_settings(SETTINGS_STORE, bindings);
}

void OperaCakeManager::load_and_apply() {
    /* Probe hardware first so we know which boards to (re)apply to. */
    detect_boards();

    std::string s;
    SettingBindings bindings;
    bindings.emplace_back(std::string_view{"cfg"}, &s);
    if (!load_settings(SETTINGS_STORE, bindings) || s.empty())
        return;  // no saved config; boards stay at their safe defaults

    const auto fields = split_string(s, ',');
    size_t fi = 0;
    auto next = [&]() -> uint32_t {
        uint32_t v = 0;
        if (fi < fields.size())
            parse_int(fields[fi], v);
        fi++;
        return v;
    };

    /* Restore per-board mode + manual ports. */
    for (uint8_t i = 0; i < max_boards; i++) {
        const uint32_t m = next();
        const uint32_t pa = next();
        const uint32_t pb = next();
        state_[i].mode = (m == static_cast<uint8_t>(Mode::Frequency)) ? Mode::Frequency : Mode::Manual;
        state_[i].PA = pa <= PB4 ? static_cast<uint8_t>(pa) : static_cast<uint8_t>(PA1);
        state_[i].PB = pb <= PB4 ? static_cast<uint8_t>(pb) : static_cast<uint8_t>(PB1);
    }

    /* Restore the range table (read all 8 slots so the cursor stays aligned). */
    ranges_.clear();
    uint32_t count = next();
    if (count > max_ranges)
        count = max_ranges;
    for (uint32_t i = 0; i < max_ranges; i++) {
        const uint32_t mn = next();
        const uint32_t mx = next();
        const uint32_t port = next();
        if (i < count)
            ranges_.push_back({static_cast<uint16_t>(mn),
                               static_cast<uint16_t>(mx),
                               static_cast<uint8_t>(port & 0x07)});
    }

    /* Re-apply manual routing to present boards. */
    for (uint8_t i = 0; i < max_boards; i++) {
        if (state_[i].present && state_[i].mode == Mode::Manual)
            activate_ports(i, state_[i].PA, state_[i].PB);
    }

    current_range_ = INVALID_RANGE;
    recompute_frequency_flag();
}

}  // namespace operacake
