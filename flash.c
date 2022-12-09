/*
 * Copyright 2014 Dirk Eibach <eibach@gdsys.de>
 *           2022 Alex Kiselev <a.kiselev@volz-servos.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "defines.h"
#include "c2tool.h"
#include "progress.h"

static void flash_section(struct c2tool_state *state, uint32_t flash_addr, uint32_t size, uint8_t *data)
{
	while (size) {
		int res;

		res = flash_chunk(state, flash_addr, size, data);
		if (res < 0) {
			fprintf(stderr, "flash chunk at %08x failed.\n", flash_addr);
			break;
		}
		flash_addr += res;
		size -= res;
		data += res;
	}
}

static int c2_flash_file(struct c2tool_state *state, const char *filename)
{

	return 0;
}

int handle_flash(struct c2tool_state *state, int argc, char **argv)
{
	char *filename;

	if (argc == 1)
		filename = argv[0];
	else
		return 1;

	if (c2family_setup(state) < 0)
		return -EIO;

	return c2_flash_file(state, filename);
}

COMMAND(flash, "<file>", handle_flash, "Write file to flash memory of connected device.");
