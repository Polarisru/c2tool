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
#include "ihex.h"
#include "progress.h"
#include "log.h"

static void verify_section(struct c2tool_state *state, uint32_t flash_addr,
                           uint32_t size, uint8_t *data)
{
	unsigned char buf[256];
	uint16_t p_counter = 0;
	uint16_t pages;

  // Find the number of pages
  pages = size / sizeof(buf);
  if (size % sizeof(buf) != 0)
    pages++;
  p_counter = 0;

  PROGRESS_Print(0, pages, "Verifying: ", '#');

	while (size) {
		int res;
		unsigned int chunk = size > sizeof(buf) ? sizeof(buf) : size;

		res = c2_flash_read(state, flash_addr, chunk, buf);
		if (res)
			return;
		if (memcmp(&data[flash_addr], buf, chunk) != 0) {
			//printf("verify failed in %d byte chunk at %08x\n", chunk, flash_addr);
			printf("\n");
			for (int i = 0; i < 16; i++)
      {
        printf("%02X->%02X\n", buf[i], data[flash_addr + i]);
      }
			LOG_Print(LOG_LEVEL_ERROR, "Failed");
      break;
		}

		flash_addr += chunk;
		size -= chunk;
		data += chunk;
    p_counter++;
    PROGRESS_Print(p_counter, pages, "Verifying: ", '#');
	}
}

static int c2_verify_file(struct c2tool_state *state, const char *filename)
{
  FILE *fp;
  uint8_t *fdata;
  //uint8_t errCode;
  uint32_t max_addr, min_addr;

  fdata = malloc(FLASH_MAX_LEN);
  if (!fdata)
  {
    LOG_Print(LOG_LEVEL_ERROR, "Unable to allocate %d bytes\n", FLASH_MAX_LEN);
    return false;
  }
  if ((fp = fopen(filename, "rt")) == NULL)
  {
    LOG_Print(LOG_LEVEL_ERROR, "Unable to open file: %s", filename);
    free(fdata);
    return false;
  }
  max_addr = 0;
  min_addr = 0;
  if (IHEX_ReadFile(fp, fdata, FLASH_MAX_LEN, &min_addr, &max_addr) != IHEX_ERROR_NONE) {

  }
  fclose(fp);

  verify_section(state, min_addr, max_addr - min_addr, fdata);
  free(fdata);

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
