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
#include <ipxe/uri.h>
#include <ipxe/console.h>
#include <ipxe/image.h>
#include <ipxe/pixbuf.h>
#include <usr/imgmgmt.h>
#include <ipxe/image.h>
#include <ipxe/vesafb_io.h>
#include <ipxe/resource.h>

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
			struct vbe_mode_info result = { 0, };
			return result;
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

void vesafb_draw_putchar ( const char c, const int x, const int y,
						   const int rgbCode ) {
	unsigned char txt;
	int w, i, j;

	/* vesafb init */
	vesafb_draw_init();

	if ( c < '&' || c > 'z' ) return;
	w = (c - '&') * 16;
	/* start index of font */
	for ( i = 0; i < 16; i++) {
		txt = font_data[w + i];
		for ( j = 7; j >= 0; j--) {
			if ( ( (txt >> j) & 1 ) ) {
				vesafb_draw_pixel ( x + (7 - j), y + i, rgbCode );
			}
		}
	}
}

void vesafb_draw_text ( const char *str, const int x, const int y,
                        const int rgbCode ) {
	int i, len = strlen(str);
	for ( i = 0; i < len; i++) {
		vesafb_draw_putchar ( str[i], x + i * 10, y, rgbCode );
	}
}

/**
 * Draw image file for PNG type in VESA/VGA Mode
 *
 * @v	path		virtual file name or NULL
 * @v	binary		data of real png binary image
 * @v	len			size of real png image
 * @v	x			abstract x resolution position in screen
 * @v	y			abstract y resolution position in screen
 * @v	w			width of png image
 * @v	h			height of png image
 * @ret	rc			return of status code
 */
int vesafb_draw_png ( const char *path, char* binary, int len,
					  const int x, const int y,
					  const int w __unused, const int h __unused ) {
    struct image *image = NULL;
    struct console_configuration config;
    struct uri *uri;
	unsigned int i, j;
	int rc = 0;

    /* vesafb init */
    vesafb_draw_init();

    uri = parse_uri ( path );
    image = alloc_image ( uri );
    if ( ! image ) {
        goto err_alloc_image;
    }

    image->name = zalloc ( strlen ( path ) + 1 );
    strcpy ( image->name, path );

    image->data = (userptr_t)binary;
    image->len = len;

    /* Register image */
    if ( ( rc = register_image ( image ) ) != 0 ) {
        printf ( "Could not register image: %s\n", strerror ( rc ) );
        goto err_register_image;
    }

    if ( ! ( image->type && image->type->pixbuf ) ) {
        printf ( "Uninitialise image->type->pixbuf () !!\n" );
        goto err_pixbuf_image;
    }

    /* Convert to pixel buffer */
    if ( ( rc = image_pixbuf ( image, &config.pixbuf ) ) != 0){
        printf ( "Could not use picture: %s\n", strerror ( rc ) );
        goto err_pixbuf_image;
    }

    unsigned int *pixels = (unsigned int *)config.pixbuf->data;
    for ( i = 0; i < config.pixbuf->height; i++) {
        for (j = 0; j < config.pixbuf->width; j++) {
            vesafb_draw_pixel ( j + x, i + y, *pixels );
            pixels++;
        }
    }

    unregister_image ( image );
err_register_image:
err_pixbuf_image:
    //image_put ( image );
err_alloc_image:
    uri_put ( uri );
	return rc;
}

void vesafb_draw_pixel_swap ( const int x1, const int y1,
							  const int x2, const int y2,
							  const int w,  const int h ) {
	struct vbe_mode_info mode;
	unsigned int *swap_mem1, *swap_mem2;
	unsigned int temp;
	int i, j;

	/* initialise vesafb draw */
	vesafb_draw_init();

	mode = vesafb_get_mode_info();

	swap_mem1 = vesa_io_memory + mode.x_resolution * y1 + x1;
	swap_mem2 = vesa_io_memory + mode.x_resolution * y2 + x2;
	for ( i = 0; i < h; i++ ) {
		for ( j = 0; j < w; j++ ) {
			temp = *swap_mem1;
			*swap_mem1 = *swap_mem2;
			*swap_mem2 = temp;
			swap_mem1++;
			swap_mem2++;
		}
		swap_mem1 += mode.x_resolution - w;
		swap_mem2 += mode.x_resolution - w;
	}
}

/**
 * draw pixel
 *
 * @v	x			Start of X pointer for pixel
 * @v	y			Start of Y pointer for pixel
 * @v	rgbCode		RGB Color of pixel
 */ 
void vesafb_draw_pixel ( const int x, const int y, const int rgbCode ) {
	struct vbe_mode_info mode;
	unsigned int *current_memory;

	if ( x < 0 || y < 0 ) return;

	/* initialise vesafb draw */
	vesafb_draw_init();

	mode = vesafb_get_mode_info();

	current_memory = vesa_io_memory + mode.x_resolution * y + x;
	*current_memory = ALPHA_BLEND(*current_memory, rgbCode);
}

/**
 * clear screen for vesa/vga frame buffer
 *
 * @v	rgbCode		RGB Color to clear screen
 */
void vesafb_clear ( const int rgbCode ) {
	struct vbe_mode_info mode;
	unsigned int *current_memory;
	int i, j;

	/* initialise vesafb draw */
	vesafb_draw_init();

	mode = vesafb_get_mode_info();

	current_memory = vesa_io_memory;
	for ( i = 0; i < mode.y_resolution; i++ ) {
		for ( j = 0; j < mode.x_resolution; j++ ) {
			*current_memory = rgbCode;
			current_memory++;
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
	int x1, x2, x3, x4;
	int y1, y2, y3, y4;

	x1 = start_x; y1 = start_y;
	x2 = end_x;   y2 = start_y;
	x3 = end_x;   y3 = end_y;
	x4 = start_x; y4 = end_y;

	/* draw lines */
	vesafb_draw_line(x1, y1, x2, y2, rgbCode);
	vesafb_draw_line(x2, y2, x3, y3, rgbCode);
	vesafb_draw_line(x1, y1, x4, y4, rgbCode);
	vesafb_draw_line(x3, y3, x4, y4, rgbCode);
}

/**
 * draw border for circle
 *
 * @v	rx			Center point of X pointer for circle
 * @v	ry			Center point of Y pointer for circle
 * @v	width		Width of circle
 * @v	height		Height of circle
 * @v	rgbCode		RGB Color of circle
 */
void vesafb_draw_circle ( const int rx, const int ry,
						  const int width, const int height,
						  const int rgbCode __unused ) {
	int ww, hh, d, x, y;

	if ( rx < 0 || ry < 0 || width < 1 || height < 1 ) return;

	/* initialise vesafb draw */
	vesafb_draw_init();

	ww = width * width;
	hh = height * height;
	x = 0;
	y = height;
	d = (4 * hh + ww * (1 - 4 * height)) / 4;
	while ( hh * x < ww * y ) {
		++x;
		if ( d < 0 ) {
			d += hh * (2 * x + 1);
		} else {
			--y;
			d += hh * (2 * x + 1) - 2 * ww * y;
		}

		vesafb_draw_pixel(rx + x, ry + y, rgbCode);
		vesafb_draw_pixel(rx - x, ry + y, rgbCode);
		vesafb_draw_pixel(rx + x, ry - y, rgbCode);
		vesafb_draw_pixel(rx - x, ry - y, rgbCode);
	}

	x = width; y = 0;
	d = (4 * ww + hh * (1 - 4 * width)) / 4;
	while ( hh * x > ww * y ) {
		++y;
		if ( d < 0 ) {
			d += ww * (2 * y + 1);
		} else {
			--x;
			d += ww * (2 * y + 1) - 2 * hh * x;
		}

		vesafb_draw_pixel(rx + x, ry + y, rgbCode);
		vesafb_draw_pixel(rx - x, ry + y, rgbCode);
		vesafb_draw_pixel(rx + x, ry - y, rgbCode);
		vesafb_draw_pixel(rx - x, ry - y, rgbCode);
	}
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
						const int rgbCode ) {
	int dx, dy, addx, addy, x, y, i, cnt;

	if (start_x < 0 || start_y < 0 || end_x < 0 || end_y < 0) return;

	/* initialise vesafb draw */
	vesafb_draw_init();

	cnt = 0;
	dx = end_x - start_x;
	dy = end_y - start_y;
	addx = 1;
	addy = 1;
	x = start_x;
	y = start_y;

	if ( dx < 0 ) { addx = -1; dx = -dx; }
	if ( dy < 0 ) { addy = -1; dy = -dy; }
	if ( dx >= dy ) {
		for ( i = 0; i < dx; i++ ) {
			x += addx;
			cnt += dy;
			if ( cnt >= dx ) {
				y += addy;
				cnt -= dx;
			}
			// x, y
			vesafb_draw_pixel(x, y, rgbCode);
		}
	} else {
		for ( i = 0; i < dy; i++ ) {
			y += addy;
			cnt += dx;
			if ( cnt >= dy ) {
				x += addx;
				cnt -= dy;
			}
			// x, y
			vesafb_draw_pixel(x, y, rgbCode);
		}
	}
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
	int i, j;

	if (start_x < 0 || start_y < 0 || end_x < 0 || end_y < 0) return;

	/* initialise vesafb draw */
	vesafb_draw_init();

	// move start pointer
	for ( i = start_y; i < end_y; i++ ) {
		for ( j = start_x; j < end_x; j++ ) {
			vesafb_draw_pixel(j, i, rgbCode);
		}
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
							   const int width, const int height,
							   const int rgbCode __unused ) {
	int ww, hh, d, x, y, i;

	if ( rx < 0 || ry < 0 || width < 1 || height < 1 ) return;

	/* initialise vesafb draw */
	vesafb_draw_init();

	ww = width * width;
	hh = height * height;
	x = 0;
	y = height;
	d = (4 * hh + ww * (1 - 4 * height)) / 4;
	while ( hh * x < ww * y ) {
		++x;
		if ( d < 0 ) {
			d += hh * (2 * x + 1);
		} else {
			--y;
			d += hh * (2 * x + 1) - 2 * ww * y;
		}

		for ( i = rx - x; i < rx + x; i++ )
			vesafb_draw_pixel(i, ry + y, rgbCode);
		for ( i = rx - x; i < rx + x; i++ )
			vesafb_draw_pixel(i, ry - y, rgbCode);
	}

	for ( i = rx - width; i < rx + width; i++ )
		vesafb_draw_pixel(i, ry, rgbCode);

	x = width; y = 0;
	d = (4 * ww + hh * (1 - 4 * width)) / 4;
	while ( hh * x > ww * y ) {
		++y;
		if ( d < 0 ) {
			d += ww * (2 * y + 1);
		} else {
			--x;
			d += ww * (2 * y + 1) - 2 * hh * x;
		}

		for ( i = rx - x; i < rx + x; i++ )
			vesafb_draw_pixel(i, ry + y, rgbCode);
		for ( i = rx - x; i < rx + x; i++ )
			vesafb_draw_pixel(i, ry - y, rgbCode);
	}
}
