#ifndef _IPXE_VESAFB_IO_H
#define _IPXE_VESAFB_IO_H

/* ipxe vesa/vga framebuffer header */
#include <ipxe/vesafb.h>

/** Base memory buffer used for VBE calls */
union vbe_buffer {
    /** VBE controller information block */
    struct vbe_controller_info controller;
    /** VBE mode information block */
    struct vbe_mode_info mode;
};

#define ARGB_A(color) (((color) >> 24) & 0xFF)
#define ARGB_R(color) (((color) >> 16) & 0xFF)
#define ARGB_G(color) (((color) >>  8) & 0xFF)
#define ARGB_B(color) (((color) >>  0) & 0xFF)
#define ARGB(a, r, g, b) (((a) << 24) + ((r) << 16) + ((g) << 8) + b)

/* alpha blending */
#define ALPHA_BLEND_C(origin, curr, curr_a) \
	(((origin) * (curr_a) + (curr) * (100 - (curr_a)))/100 )
#define ALPHA_BLEND(origin, curr) ARGB(\
	ARGB_A(curr),\
	ALPHA_BLEND_C(ARGB_R(origin), ARGB_R(curr), ARGB_A(curr)),	\
	ALPHA_BLEND_C(ARGB_G(origin), ARGB_G(curr), ARGB_A(curr)),	\
	ALPHA_BLEND_C(ARGB_B(origin), ARGB_B(curr), ARGB_A(curr)) 	\
)

extern struct console_driver* has_vesafb ( void );
extern struct vbe_mode_info vesafb_get_mode_info ( void );

extern void vesafb_draw_init ( void );

extern void vesafb_draw_pixel ( const int x, const int y,
								const int rgbCode );
extern void vesafb_draw_rect ( const int start_x, const int start_y, 
							   const int end_x, const int end_y,
							   const int rgbCode );
extern void vesafb_draw_circle ( const int rx, const int ry,
								 const int width, const int height,
								 const int rgbCode );
extern void vesafb_draw_line ( const int start_x, const int start_y,
							   const int end_x, const int end_y,
							   const int rgbCode );

extern void vesafb_draw_rect_fill ( const int start_x, const int start_y,
							 		const int end_x, const int end_y,
							 		const int rgbCode );
extern void vesafb_draw_circle_fill ( const int rx, const int ry,
									  const int width, const int height,
							   		  const int rgbCode );

#endif
