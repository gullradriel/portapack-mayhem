/*
 * Copyright (C) 2025
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

#ifndef __EXTERNAL_APP_SLOT_H__
#define __EXTERNAL_APP_SLOT_H__

#include <cstdint>
#include "../flashsize.h"

namespace portapack {
namespace external_app_slot {

constexpr uint32_t kFlashSlotMagic = 0x534C4F54;  // "SLOT"
constexpr uint32_t kFlashSlotVersion = 1;
constexpr uint32_t kFlashSlotSize = 96 * 1024;
constexpr uint32_t kFlashSlotOffset = (FLASH_SIZE_MB * 1024 * 1024) - kFlashSlotSize;
constexpr uint32_t kFlashSlotPageSize = 256;
constexpr uint32_t kFlashSlotSectorSize = 4096;

struct FlashSlotRequest {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    uint32_t slot_offset;
    uint32_t slot_size;
    uint32_t payload_size;
    char16_t path[220];
};

}  // namespace external_app_slot
}  // namespace portapack

#endif /*__EXTERNAL_APP_SLOT_H__*/
