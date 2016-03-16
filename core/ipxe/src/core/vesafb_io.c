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

/** @file
 *
 * VESA/VGA Graphics API
 *
 */

#include "stddef.h"
#include <ipxe/console.h>
#include <ipxe/vesafb_io.h>

static unsigned int *vesa_io_memory = NULL;

/**
 * Get vesafb console driver
 *
 * @ret vesafb      Return console_driver pointer
 */
struct console_driver* has_vesafb ( void ) {
    struct console_driver* vesafb;

    for_each_table_entry ( vesafb, CONSOLES ) {
        if ( vesafb->usage & CONSOLE_VESAFB ) {
            return vesafb;
        }
    }
    return NULL;
}

/**
 * Get vesafb mode information
 *
 * @ret mode      Return vbe_mode_info value
 */
struct vbe_mode_info vesafb_get_mode_info ( void ) {
	static struct vbe_mode_info *mode = NULL;
	struct console_configuration config;
	struct console_driver *vesafb;

	if ( mode == NULL ) {
		vesafb = has_vesafb();
		if ( vesafb && vesafb->configure ) {
			vesafb->configure ( &config );
			mode = (struct vbe_mode_info *)config.data;
		} else {
			return (struct vbe_mode_info){ 0, };
		}
	}
	return *mode;
}

/**
 * Initialise vesafb console driver
 */
void vesafb_draw_init ( void ) {
	struct vbe_mode_info *mode;
	struct console_configuration config;
	struct console_driver *vesafb;

	/* VESA Video IO Memory Setting */
	if ( vesa_io_memory == NULL ) {
		vesafb = has_vesafb();
		if ( vesafb && vesafb->configure ) {
			vesafb->configure ( &config );
			mode = (struct vbe_mode_info *)config.data;

			// setting start address of video memory
			vesa_io_memory = (unsigned int *) phys_to_virt (
				mode->phys_base_ptr
			);
		}
	}
}

/**
 * draw border for rectangle
 *
 * @v   start_x		Start of X pointer for rectangle
 * @v	start_y		Start of Y pointer for rectangle
 * @v	end_x		End of X pointer for rectangle
 * @v	end_y		End of Y pointer for rectangle
 * @v	rgbCode		RGB Color of rectangle
 */
void vesafb_draw_rect ( const int start_x, const int start_y,
						const int end_x, const int end_y,
						const int rgbCode ) {

	struct vbe_mode_info mode;
	unsigned int *current_memory;
	int i, j;

	/* initialise vesafb draw */
	vesafb_draw_init();

	mode = vesafb_get_mode_info();

	/* move start pointer */
	current_memory = vesa_io_memory + start_y * mode.x_resolution + start_x;
	for ( i = start_y; i < end_y; i++ ) {
		for ( j = start_x; j < end_x; j++ ) {
			/* alpha blending */
			int alpha = ARGB_A(rgbCode);

			int cRed = ARGB_R(rgbCode) * (100 - alpha);
			int cGreen = ARGB_G(rgbCode) * (100 - alpha);
			int cBlue = ARGB_B(rgbCode) * (100 - alpha);

			int red = ARGB_R(*current_memory) * alpha;
			int green = ARGB_G(*current_memory) * alpha;
			int blue = ARGB_B(*current_memory) * alpha;

			/* draw border of rectangle */
			if (i == start_y || i == end_y - 1 ||
				j == start_x || j == end_y - 1) {
				*current_memory = ARGB(
					0xFF,
					(red + cRed)/100,
					(green + cGreen)/100,
					(blue + cBlue)/100
				);
			}
			current_memory++;
		}
		current_memory += mode.x_resolution - (end_x - start_x);
	}
}

/**
 * draw border for circle
 *
 * @v	rx			Center point of X pointer for circle
 * @v	ry			Center point of Y pointer for circle
 * @v	rgbCode		RGB Color of circle
 */
void vesafb_draw_circle ( const int rx, const int ry,
						  const int rgbCode __unused ) {
	struct vbe_mode_info mode;

	if ( rx < 0 || ry < 0 ) return;

	mode = vesafb_get_mode_info();
	printf ( "unsupport function IO : %x\n", mode.phys_base_ptr );
}

/**
 * draw line from start to end
 *
 * @v   start_x		Start of X pointer for line
 * @v	start_y		Start of Y pointer for line
 * @v	end_x		End of X pointer for line
 * @v	end_y		End of Y pointer for line
 * @v	rgbCode		RGB Color of line
 */
void vesafb_draw_line ( const int start_x, const int start_y,
						const int end_x, const int end_y,
						const int rgbCode __unused ) {
	struct vbe_mode_info mode;

	if (start_x < 0 || start_y < 0 || end_x < 0 || end_y < 0) return;

	mode = vesafb_get_mode_info();
	printf ( "unsupport function IO : %x\n", mode.phys_base_ptr );
}

/**
 * draw fill for rectangle
 *
 * @v   start_x		Start of X pointer for rectangle
 * @v	start_y		Start of Y pointer for rectangle
 * @v	end_x		End of X pointer for rectangle
 * @v	end_y		End of Y pointer for rectangle
 * @v	rgbCode		RGB Color of rectangle
 */
void vesafb_draw_rect_fill ( const int start_x, const int start_y,
							 const int end_x, const int end_y,
							 const int rgbCode ) {
	struct vbe_mode_info mode;
	unsigned int *current_memory;
	int i, j;

	if (start_x < 0 || start_y < 0 || end_x < 0 || end_y < 0) return;

	/* initialise vesafb draw */
	vesafb_draw_init();

	mode = vesafb_get_mode_info();

	// move start pointer
	current_memory = vesa_io_memory + start_y * mode.x_resolution + start_x;
	for ( i = start_y; i < end_y; i++ ) {
		for ( j = start_x; j < end_x; j++ ) {
			int alpha = ARGB_A(rgbCode);

			int cRed = ARGB_R(rgbCode) * (100 - alpha);
			int cGreen = ARGB_G(rgbCode) * (100 - alpha);
			int cBlue = ARGB_B(rgbCode) * (100 - alpha);

			int red = ARGB_R(*current_memory) * alpha;
			int green = ARGB_G(*current_memory) * alpha;
			int blue = ARGB_B(*current_memory) * alpha;

			*current_memory = ARGB(
				0xFF,
				(red + cRed)/100, (green + cGreen)/100, (blue + cBlue)/100
			);
			current_memory++;
		}
		current_memory += mode.x_resolution - (end_x - start_x);
	}
}

/**
 * draw fill for circle
 *
 * @v	rx			Center point of X pointer for circle
 * @v	ry			Center point of Y pointer for circle
 * @v	rgbCode		RGB Color of circle
 */
void vesafb_draw_circle_fill ( const int rx, const int ry,
							   const int rgbCode __unused ) {
	struct vbe_mode_info mode;

	if ( rx < 0 || ry < 0 ) return;

	mode = vesafb_get_mode_info();
	printf ( "unsupport function IO : %x\n", mode.phys_base_ptr );
}
