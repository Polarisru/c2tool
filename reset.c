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

#include <stdio.h>
#include <errno.h>

#include "c2family.h"
#include "c2tool.h"

int handle_reset(struct c2tool_state *state, int argc, char **argv)
{
	struct c2interface *c2if = &state->c2if;

	c2_reset(c2if);

	return 0;
}

COMMAND(reset, NULL, handle_reset, "Reset connected device.");
