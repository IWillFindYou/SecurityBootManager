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
#include <ipxe/console.h>
#include <ipxe/io.h>
#include <ipxe/vesafb.h>
#include <ipxe/command.h>
#include <ipxe/parseopt.h>

/** Base memory buffer used for VBE calls */
union vbe_buffer {
        /** VBE controller information block */
        struct vbe_controller_info controller;
        /** VBE mode information block */
        struct vbe_mode_info mode;
};
static union vbe_buffer * __bss16 ( vbe_buf );
#define vbe_buf __use_data16 ( vbe_buf )

#define SCREEN_WIDTH  1024
#define SCREEN_HEIGHT 768
 
/** @file
 *
 * x86 VESA feature detection command
 *
 */

/** "vesa" options */
struct vesa_options {
	struct console_configuration config;
};

/** "vesa" option list */
static struct option_descriptor vesa_opts[] = {};

/** "vesa" command descriptor */
static struct command_descriptor vesa_cmd =
	COMMAND_DESC ( struct vesa_options, vesa_opts, 0, 0, NULL );

static unsigned int *vesa_io_memory = NULL;

/**
 * Get vesafb console driver
 *
 * @ret vesafb		Return console_driver pointer
 */
static struct console_driver* has_vesafb ( void ) {
	struct console_driver* vesafb;

	for_each_table_entry ( vesafb, CONSOLES ) {
		if ( vesafb->usage & CONSOLE_VESAFB ) {
			return vesafb;
		}
	}
	return NULL;
}

/**
 * The "vesa" command
 *
 * @v argc              Argument count
 * @v argv              Argument list
 * @ret rc              Return status code
 */
static int vesa_exec ( int argc, char **argv ) {
	struct vesa_options opts;
	struct vbe_mode_info *mode;
	struct console_driver *vesafb;
	int rc, i, j;

	/* Parse options */
	if ( ( rc = parse_options ( argc, argv, &vesa_cmd, &opts ) ) != 0 )
		return rc;

	opts.config.width  = SCREEN_WIDTH;
	opts.config.height = SCREEN_HEIGHT;

	vesafb = has_vesafb();
	if ( vesafb && vesafb->configure ) {
		vesafb->configure ( &opts.config );

		vbe_buf = (union vbe_buffer *)opts.config.data;
		mode = &vbe_buf->mode;

		vesa_io_memory = (unsigned int *)phys_to_virt(mode->phys_base_ptr);

		printf ( "VESA/VGA Screen resolution %d, %d\n",
				vbe_buf->mode.x_resolution, vbe_buf->mode.y_resolution );
		printf ( "VESA/VGA Video IO Memory %08x\n",
				vbe_buf->mode.phys_base_ptr );

		for (i = 0; i < vbe_buf->mode.y_resolution; i++) {
			for (j = 0; j < vbe_buf->mode.x_resolution; j++) {
				*vesa_io_memory = 0x00000000;
				vesa_io_memory++;
			}
		}
	} else {
		printf ("VESA Mode Change Failure\n");
	}
	return rc;
}

/** x86 CPU feature detection command */
struct command vesa_command __command = {
        .name = "vesafb",
        .exec = vesa_exec,
};
