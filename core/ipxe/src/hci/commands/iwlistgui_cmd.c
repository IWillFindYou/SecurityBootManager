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
#include <ipxe/image.h>
#include <ipxe/command.h>
#include <ipxe/parseopt.h>
#include <ipxe/vesafb_io.h>
#include <ipxe/pixbuf.h>
#include <usr/iwmgmt.h>
#include <usr/imgmgmt.h>
#include <ipxe/uri.h>
#include <hci/ifmgmt_cmd.h>
#include <ipxe/keys.h>
#include <ipxe/editstring.h>
#include <readline/readline.h>
#include <ipxe/resource.h>

/** @file
 *
 * Wireless GUI interface management commands
 *
 */

/** "iwlistgui" options */
struct iwlist_options {};

/** "iwlistgui" option list */
static struct option_descriptor iwlist_opts[] = {};

/**
 * "iwlist" payload
 *
 * @v netdev		Network device
 * @v opts		Command options
 * @ret rc		Return status code
 */
static int iwlist_payload ( struct net_device *netdev,
			    struct iwlist_options *opts __unused ) {
	struct net80211_device *dev = net80211_get ( netdev );

	if ( dev )
		return iwlist ( dev );

	return 0;
}

/** "vesa" command descriptor */
//static struct command_descriptor iwlist_cmd =
//    COMMAND_DESC ( struct iwlist_options, iwlist_opts, 0, 0, NULL );
static struct ifcommon_command_descriptor iwlist_cmd __unused =
	IFCOMMON_COMMAND_DESC ( struct iwlist_options, iwlist_opts,
				0, MAX_ARGUMENTS, "[<interface>...]",
				iwlist_payload, 0 );

/**
 * The "iwlist" command
 *
 * @v argc		Argument count
 * @v argv		Argument list
 * @ret rc		Return status code
 */
static int iwlist_exec ( int argc __unused, char **argv __unused ) {
	struct net80211_device *dev = net80211_get ( find_netdev ( "net0" ) );
    struct vbe_mode_info mode;
	struct edit_string string;
	struct ap_list* list;
	int rc = 0, sx = 0, ex = 0, sy = 0, ey = 0, w = 0, h = 0;
	int key;

	vesafb_draw_init();

    mode = vesafb_get_mode_info ();

	/* clear screen */
	vesafb_clear ( 0xDDDDDD );

	w = 250;
	h = 300;
	sx = (mode.x_resolution - w) / 2;
	ex = w + sx;
	sy = 350;
	ey = sy + h;
	vesafb_draw_rect( sx, sy, ex, ey, ARGB(30, 255, 255, 255) );
	vesafb_draw_rect( sx + 1, sy + 1, ex - 1, ey - 1,
		ARGB(30, 255, 255, 255) );

	rc = vesafb_draw_png ( "logo.png",  (char *)img_logo, 
		img_logo_len, (mode.x_resolution - 626) / 2, 150, 0, 0);

	rc = vesafb_draw_png ( "wifi.png",  (char *)img_wifi, 
		img_wifi_len, sx - 40, 300, 0, 0);

	rc = vesafb_draw_png ( "choice.png",  (char *)img_choice, 
		img_choice_len, ex + 10, 300, 0, 0);

	/* get ap list */
	list = get_iwlist (dev);

	int ap_count = list->len;
	int ap_index = 0;
	while ( 1 ) {
		/* keyboard input control setting */
		key = edit_string ( &string, getkey (0) );

		switch (key) {
		case KEY_UP:
			vesafb_draw_pixel_swap ( ex + 10, 300 + 20 * (ap_index + 0),
									 ex + 10, 300 + 20 * (ap_index - 1),
									 20, 20 );
			if ( ap_index > 0 )
				--ap_index;
			break;
		case KEY_DOWN:
			vesafb_draw_pixel_swap ( ex + 10, 300 + 20 * (ap_index + 0),
									 ex + 10, 300 + 20 * (ap_index + 1),
									 20, 20 );
			if ( ap_index < ap_count )
				++ap_index;
			break;
		case KEY_ENTER:
			printf ( "AP Count : %d\n", list->len );
			printf ( "AP[%d] name : %s\n", ap_index,
				list->ap[ap_index].ssid );
			break;
		}
	}

	return rc;
}

/** Wireless interface management commands */
struct command iwlistgui_commands[] __command = {
	{
		.name = "iwlistgui",
		.exec = iwlist_exec,
	},
};
