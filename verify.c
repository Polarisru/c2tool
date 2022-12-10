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

static void verify_section(struct c2tool_state *state, uint32_t flash_addr,
                           uint32_t size, uint8_t *data)
{
	unsigned char buf[256];

	while (size) {
		int res;
		unsigned int chunk = size > sizeof(buf) ? sizeof(buf) : size;

		res = c2_flash_read(state, flash_addr, chunk, buf);
		if (res)
			return;
		if (memcmp(data, buf, chunk)) {
			//printf("verify failed in %d byte chunk at %08x\n", chunk, flash_addr);
			//fsdata->errctr++;
		}

		flash_addr += chunk;
		size -= chunk;
		data += chunk;
	}

	//printf("section %s: verify ok\n", section_name);
}

static int c2_verify_file(struct c2tool_state *state, const char *filename)
{
  return 0;
}

int handle_verify(struct c2tool_state *state, int argc, char **argv)
{
	char *filename;

	if (argc == 1)
		filename = argv[0];
	else
		return 1;

	if (c2family_setup(state) < 0)
		return -EIO;

	return c2_verify_file(state, filename);
}

COMMAND(verify, "<file>", handle_verify, "Verify file to flash memory of connected device.");
