/*
 * Copyright (C) 2016 Byeoungwook Kim <quddnr145@gmail.com>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * You can also choose to distribute this program under the terms of
 * the Unmodified Binary Distribution Licence (as given in the file
 * COPYING.UBDL), provided that you have satisfied its requirements.
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <ipxe/vesafb.h>
#include <ipxe/command.h>
#include <ipxe/parseopt.h>

/** @file
 *
 * x86 VESA feature detection command
 *
 */

/** "vesa" options */
struct vesa_options {};

/** "vesa" option list */
static struct option_descriptor vesa_opts[] = {};

/** "vesa" command descriptor */
static struct command_descriptor vesa_cmd =
	COMMAND_DESC ( struct vesa_options, vesa_opts, 0, 0, NULL );

/**
 * The "vesa" command
 *
 * @v argc              Argument count
 * @v argv              Argument list
 * @ret rc              Return status code
 */
static int vesa_exec ( int argc __unused, char **argv __unused ) {
	struct vesa_options opts;
	int rc;

	/* Parse options */
	if ( ( rc = parse_options ( argc, argv, &vesa_cmd, &opts ) ) != 0 )
		return rc;

        return 0;
}

/** x86 CPU feature detection command */
struct command vesa_command __command = {
        .name = "vesa",
        .exec = vesa_exec,
};
