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

#include "ui_operacake.hpp"

#include "string_format.hpp"

#include <string>
#include <vector>

using namespace operacake;

namespace ui {

namespace {

const char* const port_names[8] = {
    "PA1", "PA2", "PA3", "PA4", "PB1", "PB2", "PB3", "PB4"};

std::string port_name(uint8_t p) {
    return p < 8 ? port_names[p] : "?";
}

}  // namespace

OperaCakeView::OperaCakeView(NavigationView& nav)
    : nav_{nav} {
    add_children({
        &text_status,
        &labels_top,
        &options_board,
        &options_mode,
        &labels_manual,
        &options_port_a,
        &options_port_b,
        &button_apply,
        &labels_freq,
        &labels_editor,
        &field_row,
        &field_min,
        &field_max,
        &options_range_port,
        &button_set_row,
        &button_del_row,
        &button_clear,
        &text_active,
        &labels_note,
    });

    /* Position and register the range-table display rows. */
    for (size_t i = 0; i < range_rows.size(); i++) {
        range_rows[i].set_parent_rect({{0, static_cast<int>((4 + i) * 16)}, {screen_width, 16}});
        add_child(&range_rows[i]);
    }

    /* Only re-probe the bus if nothing has been detected yet (e.g. boot-time
     * load_and_apply didn't run). Avoids clobbering already-active routing,
     * since detect_boards() resets every board to its hardware default. */
    if (!manager_.any_board_present())
        manager_.detect_boards();

    /* Build the board selector from the present-board list. */
    OptionsField::options_t board_options;
    for (uint8_t addr : manager_.boards())
        board_options.emplace_back(to_string_hex(0x18 + addr, 2), addr);
    if (board_options.empty())
        board_options.emplace_back("--", 0);
    options_board.set_options(board_options);
    options_board.set_selected_index(0);

    options_board.on_change = [this](size_t, OptionsField::value_t) {
        refresh_controls();
    };

    options_mode.on_change = [this](size_t, OptionsField::value_t v) {
        if (updating_ || !manager_.any_board_present())
            return;
        manager_.set_mode(selected_board(), static_cast<Mode>(v));
        manager_.save();
        update_mode_visibility();
    };

    button_apply.on_select = [this](Button&) {
        if (!manager_.any_board_present())
            return;
        manager_.set_manual_ports(
            selected_board(),
            static_cast<uint8_t>(options_port_a.selected_index_value()),
            static_cast<uint8_t>(options_port_b.selected_index_value()));
        manager_.save();
    };

    button_set_row.on_select = [this](Button&) {
        auto ranges = manager_.ranges();
        FreqRange row{
            static_cast<uint16_t>(field_min.value()),
            static_cast<uint16_t>(field_max.value()),
            static_cast<uint8_t>(options_range_port.selected_index_value())};
        size_t idx = field_row.value();
        if (idx < ranges.size())
            ranges[idx] = row;
        else if (ranges.size() < max_ranges)
            ranges.push_back(row);  // append at the next free slot
        manager_.set_ranges(ranges);
        manager_.save();
        refresh_range_list();
    };

    button_del_row.on_select = [this](Button&) {
        auto ranges = manager_.ranges();
        size_t idx = field_row.value();
        if (idx < ranges.size()) {
            ranges.erase(ranges.begin() + idx);
            manager_.set_ranges(ranges);
            manager_.save();
            refresh_range_list();
        }
    };

    button_clear.on_select = [this](Button&) {
        manager_.clear_ranges();
        manager_.save();
        refresh_range_list();
    };

    field_row.on_change = [this](int32_t) {
        refresh_range_list();  // reflect the selected row into the editor fields
    };

    field_min.set_value(100);
    field_max.set_value(200);

    refresh_controls();
}

void OperaCakeView::focus() {
    options_board.focus();
}

uint8_t OperaCakeView::selected_board() const {
    if (!manager_.any_board_present())
        return 0;
    return static_cast<uint8_t>(options_board.selected_index_value());
}

void OperaCakeView::refresh_controls() {
    updating_ = true;

    if (!manager_.any_board_present()) {
        text_status.set("No Opera Cake found");
    } else {
        std::string s = to_string_dec_uint(manager_.boards().size()) + " board";
        if (manager_.boards().size() != 1)
            s += "s";
        s += " @ 0x" + to_string_hex(0x18 + selected_board(), 2);
        text_status.set(s);

        const uint8_t board = selected_board();
        options_mode.set_by_value(static_cast<int32_t>(manager_.mode(board)));

        uint8_t pa = manager_.manual_PA(board);
        uint8_t pb = manager_.manual_PB(board);
        if (pa <= PA4)
            options_port_a.set_by_value(pa);
        if (pb >= PB1 && pb <= PB4)
            options_port_b.set_by_value(pb);
    }

    refresh_range_list();
    update_mode_visibility();

    updating_ = false;
}

void OperaCakeView::update_mode_visibility() {
    const bool present = manager_.any_board_present();
    const bool manual = present &&
                        manager_.mode(selected_board()) == Mode::Manual;
    const bool freq = present && !manual;

    /* Board/mode selectors stay visible regardless so focus always has
     * somewhere to land (even on the "No Opera Cake found" screen). */

    /* Manual panel. */
    labels_manual.hidden(!manual);
    options_port_a.hidden(!manual);
    options_port_b.hidden(!manual);
    button_apply.hidden(!manual);

    /* Frequency panel. */
    labels_freq.hidden(!freq);
    labels_editor.hidden(!freq);
    field_row.hidden(!freq);
    field_min.hidden(!freq);
    field_max.hidden(!freq);
    options_range_port.hidden(!freq);
    button_set_row.hidden(!freq);
    button_del_row.hidden(!freq);
    button_clear.hidden(!freq);
    text_active.hidden(!freq);
    for (auto& row : range_rows)
        row.hidden(!freq);

    set_dirty();
}

void OperaCakeView::refresh_range_list() {
    const auto& ranges = manager_.ranges();
    for (size_t i = 0; i < range_rows.size(); i++) {
        if (i < ranges.size()) {
            const auto& r = ranges[i];
            std::string line = to_string_dec_uint(i) + ": " +
                               to_string_dec_uint(r.min_mhz) + "-" +
                               to_string_dec_uint(r.max_mhz) + " " +
                               port_name(r.portA);
            range_rows[i].set(line);
        } else {
            range_rows[i].set("");
        }
    }

    /* Load the currently selected row into the editor fields. */
    size_t idx = field_row.value();
    if (idx < ranges.size()) {
        field_min.set_value(ranges[idx].min_mhz);
        field_max.set_value(ranges[idx].max_mhz);
        options_range_port.set_by_value(ranges[idx].portA);
    }
}

void OperaCakeView::update_active_label() {
    if (text_active.hidden())
        return;

    std::string s;
    const uint8_t cur = manager_.current_range();
    const auto& ranges = manager_.ranges();
    if (cur < ranges.size()) {
        const auto& r = ranges[cur];
        s = "Active: " + port_name(r.portA) + "  " +
            to_string_dec_uint(r.min_mhz) + "-" + to_string_dec_uint(r.max_mhz);
    } else {
        s = "Active: --";
    }

    if (s != active_cache_) {
        active_cache_ = s;
        text_active.set(s);
    }
}

}  // namespace ui
