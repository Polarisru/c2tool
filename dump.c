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
  uint8_t *fdata;
  FILE *fp;
  bool res = false;
  uint16_t pages;
  uint16_t p_counter;
  uint32_t f_size;

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

  fdata = malloc(FLASH_MAX_LEN);
  if (!fdata)
  {
    LOG_Print(LOG_LEVEL_ERROR, "Unable to allocate %d bytes", (int)len);
    return false;
  }
  memset(fdata, 0xff, len);
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

    f_size = offset + len;

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
        memcpy(buf, &fdata[offset], chunk);
        errors = 0;
        //print_hex_dump("", DUMP_PREFIX_HEX, offset, 16, 1, buf, chunk, 0);
        offset += chunk;
        len -=chunk;
        p_counter++;
        PROGRESS_Print(p_counter, pages, "Reading: ", '#');
      }
    }
    if (IHEX_WriteFile(fp, fdata, f_size) != IHEX_ERROR_NONE)
    {
      LOG_Print(LOG_LEVEL_ERROR, "Can not write to file %s", filename);
    }
  }

	return 0;
}

COMMAND(dump, "[offset] [len] <file>", handle_dump, "Dump flash memory of connected device to a HEX file.");
