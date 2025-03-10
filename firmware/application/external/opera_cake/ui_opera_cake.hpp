/*
 * copyleft spammingdramaqueen
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

#ifndef __UI_OPERA_CAKE_H__
#define __UI_OPERA_CAKE_H__

#include "ui_navigation.hpp"
#include "operacake.h"

namespace ui::external_app::opera_cake {

class OperaCakeView : public View {
   public:
    OperaCakeView(NavigationView& nav);
    std::string title() const override { return "OperaCake"; };

   private:
    NavigationView& nav_;
    uint8_t opera_cake_mode = 0;

    SettingsStore opera_cake_setting{
        "opera_cake_settings"sv,
        {{"opera_cake_mode"sv, &opera_cake_mode}}};

    void focus() override;

    Labels labels{
        {{0 * 8, 0 * 16}, "Opera Cake configuration:", Theme::getInstance()->fg_light->foreground}};
};

}  // namespace ui::external_app::opera_cake

#endif  // __UI_OPERA_CAKE_H__
