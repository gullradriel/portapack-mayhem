#!/usr/bin/env python3

#
# Copyright (C) 2024 Mark Thompson
#
# This file is part of PortaPack.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

# external app address ranges below must match those in linker file "external.ld"
maximum_application_size = 32*1024
external_apps_address_start = 0xADB00000
external_apps_address_end = 0xADF00000
external_app_slot_size = 96*1024

def _read_flash_size_mb():
	try:
		import re
		from pathlib import Path
		flashsize_path = Path(__file__).resolve().parents[2] / "flashsize.h"
		if not flashsize_path.exists():
			return 1
		content = flashsize_path.read_text()
		match = re.search(r"#define\\s+FLASH_SIZE_MB\\s+(\\d+)", content)
		if match:
			return int(match.group(1))
	except Exception:
		pass
	return 1

external_app_slot_offset = (_read_flash_size_mb() * 1024 * 1024) - external_app_slot_size
external_app_slot_address = 0x14000000 + external_app_slot_offset
