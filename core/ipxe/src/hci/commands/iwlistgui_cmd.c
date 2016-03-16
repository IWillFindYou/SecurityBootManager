/*
 * Copyright (C) 2016 Byeoungwook Kim <quddnr145@gmail.com>.
 * Based iwmgmt_cmd.d
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
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <ipxe/netdevice.h>
#include <ipxe/net80211.h>
#include <ipxe/console.h>
#include <ipxe/command.h>
#include <ipxe/parseopt.h>
#include <ipxe/vesafb_io.h>
#include <usr/iwmgmt.h>
#include <hci/ifmgmt_cmd.h>

/** @file
 *
 * Wireless GUI interface management commands
 *
 */

/** "iwlistgui" options */
struct iwlist_options {};

/** "iwlistgui" option list */
static struct option_descriptor iwlist_opts[] = {};

/** "vesa" command descriptor */
static struct command_descriptor iwlist_cmd =
    COMMAND_DESC ( struct iwlist_options, iwlist_opts, 0, 0, NULL );

/**
 * The "iwlist" command
 *
 * @v argc		Argument count
 * @v argv		Argument list
 * @ret rc		Return status code
 */
static int iwlist_exec ( int argc, char **argv ) {
	struct iwlist_options opts;
    int rc;

    /* Parse options */
    if ( ( rc = parse_options ( argc, argv, &iwlist_cmd, &opts ) ) != 0 )
        return rc;

	vesafb_draw_rect_fill( 10, 10, 100, 100, ARGB(0, 0, 0, 255) );
	vesafb_draw_rect_fill( 60, 60, 150, 150, ARGB(40, 255, 0, 0) );

	vesafb_draw_rect( 200, 200, 300, 300, ARGB(0, 255, 0, 0) );
	vesafb_draw_line( 300, 350, 450, 400, ARGB(0, 255, 0, 0) );

	return rc;
}

/** Wireless interface management commands */
struct command iwlistgui_commands[] __command = {
	{
		.name = "iwlistgui",
		.exec = iwlist_exec,
	},
};
