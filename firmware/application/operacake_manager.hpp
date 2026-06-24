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

/*
 * Opera Cake antenna switch controller (I2C / register control only).
 *
 * SAFETY: This driver talks to the on-board PCA9557 I2C GPIO expander ONLY.
 * It NEVER touches the LPC43xx GPIO pins shared with the PortaPack, so it is
 * safe to use with a PortaPack mounted on top of the HackRF + Opera Cake stack.
 * "Time mode" / pseudo-doppler (which drive the board over those shared GPIO
 * lines) are intentionally NOT implemented here and must never be added.
 *
 * Register bit math ported from greatscottgadgets/hackrf
 * firmware/common/operacake.c.
 */

#ifndef __OPERACAKE_MANAGER_HPP__
#define __OPERACAKE_MANAGER_HPP__

#include <cstddef>
#include <cstdint>
#include <vector>

namespace operacake {

/* Secondary-port enum values. These MUST match HackRF's
 * enum operacake_ports (host/libhackrf/src/hackrf.h) exactly. */
enum Port : uint8_t {
    PA1 = 0,
    PA2 = 1,
    PA3 = 2,
    PA4 = 3,
    PB1 = 4,
    PB2 = 5,
    PB3 = 6,
    PB4 = 7,
};

/* NB: NO Time mode. The GPIO/pin-control path required for time mode conflicts
 * with the PortaPack and is deliberately unreachable from this firmware. */
enum class Mode : uint8_t {
    Manual = 0,
    Frequency = 1,
};

constexpr size_t max_boards = 8;  // I2C addresses 0x18..0x1F
constexpr size_t max_ranges = 8;  // matches HackRF MAX_OPERACAKE_RANGES

/* A frequency band -> A-side port mapping. The B-side port mirrors A as
 * (portA + 4) % 8, matching HackRF frequency ("Opera Glasses") mode. */
struct FreqRange {
    uint16_t min_mhz;
    uint16_t max_mhz;
    uint8_t portA;
};

class OperaCakeManager {
   public:
    static OperaCakeManager& instance();

    /* Probe all 8 addresses; (re)populate the present-board list and reset each
     * present board to the safe default (register control, PA1/PB1, LEDs on).
     * Returns true if at least one board is present. */
    bool detect_boards();

    const std::vector<uint8_t>& boards() const { return present_boards_; }
    bool board_present(uint8_t board) const;
    bool any_board_present() const { return !present_boards_.empty(); }

    /* The only switching primitive. Validates PA/PB and issues a single I2C
     * write to the OUTPUT register. Returns 0 on success, non-zero on a bad
     * combination or I2C failure. */
    uint8_t activate_ports(uint8_t board, uint8_t PA, uint8_t PB);

    Mode mode(uint8_t board) const;
    void set_mode(uint8_t board, Mode mode);

    void set_manual_ports(uint8_t board, uint8_t PA, uint8_t PB);
    uint8_t manual_PA(uint8_t board) const;
    uint8_t manual_PB(uint8_t board) const;

    /* Frequency-mode range table (shared by all frequency-mode boards). */
    void clear_ranges();
    uint8_t add_range(uint16_t min_mhz, uint16_t max_mhz, uint8_t portA);
    const std::vector<FreqRange>& ranges() const { return ranges_; }
    void set_ranges(const std::vector<FreqRange>& ranges);

    /* Called by the radio tune hook (thread context only). Finds the matching
     * range (last range = catch-all) and switches the frequency-mode boards,
     * but only when the active band actually changes. Cheap no-op when no board
     * is in Frequency mode. */
    void on_frequency_changed(uint64_t freq_hz);
    uint8_t current_range() const { return current_range_; }

    /* Cheap early-out flag for the tune hook. */
    bool any_frequency_mode() const { return any_frequency_mode_; }

    /* Persistence (SD card .ini via app_settings). */
    void save() const;
    void load_and_apply();

   private:
    OperaCakeManager() = default;

    bool write_reg(uint8_t board, uint8_t reg, uint8_t value);
    bool read_reg(uint8_t board, uint8_t reg, uint8_t& out);
    void recompute_frequency_flag();

    struct BoardState {
        bool present = false;
        Mode mode = Mode::Manual;
        uint8_t PA = PA1;
        uint8_t PB = PB1;
    };

    BoardState state_[max_boards]{};
    std::vector<uint8_t> present_boards_{};
    std::vector<FreqRange> ranges_{};
    uint8_t current_range_ = 0xFF;  // INVALID_RANGE
    bool any_frequency_mode_ = false;
};

}  // namespace operacake

#endif /*__OPERACAKE_MANAGER_HPP__*/
