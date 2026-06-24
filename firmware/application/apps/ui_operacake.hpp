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

#ifndef __UI_OPERACAKE_HPP__
#define __UI_OPERACAKE_HPP__

#include "operacake_manager.hpp"

#include "ui.hpp"
#include "ui_navigation.hpp"
#include "ui_widget.hpp"

#include <array>

namespace ui {

class OperaCakeView : public View {
   public:
    OperaCakeView(NavigationView& nav);

    void focus() override;
    std::string title() const override { return "Opera Cake"; };

   private:
    NavigationView& nav_;

    /* The board currently selected for editing in the UI (0..7). */
    uint8_t selected_board() const;

    void refresh_controls();        // sync widgets from manager state
    void update_mode_visibility();  // show/hide the Manual vs Frequency panel
    void refresh_range_list();      // redraw the range table rows
    void update_active_label();     // live "active range" indicator

    operacake::OperaCakeManager& manager_{operacake::OperaCakeManager::instance()};

    /* Cache so the per-frame live label only repaints when it changes. */
    std::string active_cache_{};

    /* True while widgets are being synced from state, so the OptionsField
     * on_change handlers ignore the programmatic set_by_value() callbacks. */
    bool updating_{false};

    /* ---- Always-visible header ----------------------------------------- */
    Text text_status{
        {0, 1 * 16, screen_width, 16},
        ""};

    Labels labels_top{
        {{0, 2 * 16}, "Board:", Theme::getInstance()->fg_light->foreground},
        {{15 * 8, 2 * 16}, "Mode:", Theme::getInstance()->fg_light->foreground}};

    OptionsField options_board{
        {7 * 8, 2 * 16},
        4,
        {}};

    OptionsField options_mode{
        {20 * 8, 2 * 16},
        9,
        {{"Manual", static_cast<int32_t>(operacake::Mode::Manual)},
         {"Frequency", static_cast<int32_t>(operacake::Mode::Frequency)}}};

    /* ---- Manual panel --------------------------------------------------- */
    Labels labels_manual{
        {{0, 3 * 16}, "Manual ports", Theme::getInstance()->fg_light->foreground},
        {{0, 4 * 16}, "A side:", Theme::getInstance()->fg_light->foreground},
        {{0, 5 * 16}, "B side:", Theme::getInstance()->fg_light->foreground}};

    OptionsField options_port_a{
        {8 * 8, 4 * 16},
        3,
        {{"PA1", operacake::PA1},
         {"PA2", operacake::PA2},
         {"PA3", operacake::PA3},
         {"PA4", operacake::PA4}}};

    OptionsField options_port_b{
        {8 * 8, 5 * 16},
        3,
        {{"PB1", operacake::PB1},
         {"PB2", operacake::PB2},
         {"PB3", operacake::PB3},
         {"PB4", operacake::PB4}}};

    Button button_apply{
        {0, 6 * 16, 12 * 8, 28},
        "Apply"};

    /* ---- Frequency panel ------------------------------------------------ */
    Labels labels_freq{
        {{0, 3 * 16}, "Ranges MHz, last=default", Theme::getInstance()->fg_light->foreground}};

    std::array<Text, operacake::max_ranges> range_rows{};

    Labels labels_editor{
        {{0, 12 * 16}, "Row Min  Max  Port", Theme::getInstance()->fg_light->foreground}};

    NumberField field_row{
        {0, 13 * 16},
        1,
        {0, operacake::max_ranges - 1},
        1,
        ' '};

    NumberField field_min{
        {4 * 8, 13 * 16},
        4,
        {0, 9999},
        1,
        ' '};

    NumberField field_max{
        {9 * 8, 13 * 16},
        4,
        {0, 9999},
        1,
        ' '};

    OptionsField options_range_port{
        {14 * 8, 13 * 16},
        3,
        {{"PA1", operacake::PA1},
         {"PA2", operacake::PA2},
         {"PA3", operacake::PA3},
         {"PA4", operacake::PA4}}};

    Button button_set_row{
        {0, 14 * 16, 9 * 8, 28},
        "Set"};

    Button button_del_row{
        {10 * 8, 14 * 16, 9 * 8, 28},
        "Del"};

    Button button_clear{
        {20 * 8, 14 * 16, 9 * 8, 28},
        "Clear"};

    Text text_active{
        {0, 16 * 16, screen_width, 16},
        ""};

    /* ---- Always-visible footer ----------------------------------------- */
    Labels labels_note{
        {{0, 19 * 16}, "I2C-only, PortaPack-safe", Theme::getInstance()->fg_dark->foreground}};

    MessageHandlerRegistration message_handler_frame_sync{
        Message::ID::DisplayFrameSync,
        [this](const Message* const) {
            this->update_active_label();
        }};
};

}  // namespace ui

#endif /*__UI_OPERACAKE_HPP__*/
