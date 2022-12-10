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
#include "c2family.h"
#include "c2tool.h"
#include "hexdump.h"
#include "progress.h"
#include "log.h"
#include "ihex.h"

int handle_dump(struct c2tool_state *state, int argc, char **argv)
{
	uint8_t buf[256];
	char *filename;
	uint32_t offset = 0;
	uint32_t len = sizeof(buf);
	char *end;
	uint32_t errors = 0;
  FILE *fp;
  int res = 0;
  uint16_t pages;
  uint16_t p_counter;

	if (argc > 1) {
		offset = strtoul(argv[0], &end, 0);
		if (*end == '\0') {
			argv++;
			argc--;
		} else {
			return 1;
		}
	}

	if (argc > 1) {
		len = strtoul(argv[0], &end, 0);
		if (*end == '\0') {
			argv++;
			argc--;
		} else {
			return 1;
		}
	}

	if (argc == 1) {
		filename = argv[0];
	} else {
	  return 1;
	}

  if ((fp = fopen(filename, "w")) == NULL)
  {
    LOG_Print(LOG_LEVEL_ERROR, "Unable to open file: %s", filename);
  } else
  {
    // Find the number of pages
    pages = len / sizeof(buf);
    if (len % sizeof(buf) != 0)
      pages++;
    p_counter = 0;

    PROGRESS_Print(0, pages, "Reading: ", '#');

    while (len) {
      uint32_t chunk = len > sizeof(buf) ? sizeof(buf) : len;

      if (c2_flash_read(state, offset, chunk, buf) < 0) {
        errors++;
        c2_halt(&state->c2if);
        if (errors > 5) {
          return 0;
        }
      } else {
        errors = 0;
        //print_hex_dump("", DUMP_PREFIX_HEX, offset, 16, 1, buf, chunk, 0);
        if (IHEX_WriteData(fp, buf, (uint32_t)p_counter * sizeof(buf), chunk) == false) {
          LOG_Print(LOG_LEVEL_ERROR, "Can not write to file %s", filename);
          res = 1;
          break;
        }
        offset += chunk;
        len -=chunk;
        p_counter++;
        PROGRESS_Print(p_counter, pages, "Reading: ", '#');
      }
    }
    IHEX_WriteEnd(fp);
    fclose(fp);
  }

	return res;
}

COMMAND(dump, "[offset] [len] <file>", handle_dump, "Dump flash memory of connected device to a HEX file.");
