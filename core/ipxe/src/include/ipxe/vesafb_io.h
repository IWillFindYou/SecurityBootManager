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

extern struct console_driver* has_vesafb ( void );
extern struct vbe_mode_info vesafb_get_mode_info ( void );
extern void vesafb_draw_init ( void );
extern void vesafb_draw_rect ( const int start_x, const int start_y, 
							   const int end_x, const int end_y,
							   const int rgbCode );

#endif
