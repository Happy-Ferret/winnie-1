#ifdef WINNIE_FBDEV
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

#include <linux/fb.h>

#include "gfx.h"

#define FRAMEBUFFER_SIZE(xsz, ysz, bpp) ((xsz) * (ysz) * (bpp) / CHAR_BIT)

struct Graphics {
	unsigned char *framebuffer;
	int dev_fd;
	Rect screen_rect;
	Rect clipping_rect;
	int color_depth;
	Pixmap *pixmap;
};

static Graphics *gfx;

bool init_gfx()
{
	if(!(gfx = (Graphics*)malloc(sizeof *gfx))) {
		return false;
	}

	gfx->dev_fd = -1;

	if((gfx->dev_fd = open("/dev/fb0", O_RDWR)) == -1) {
		fprintf(stderr, "Cannot open /dev/fb0 : %s\n", strerror(errno));
		return false;
	}

	fb_var_screeninfo sinfo;
	if(ioctl(gfx->dev_fd, FBIOGET_VSCREENINFO, &sinfo) == -1) {
		close(gfx->dev_fd);
		gfx->dev_fd = -1;
		fprintf(stderr, "Unable to get screen info : %s\n", strerror(errno));
		return false;
	}

	printf("width : %d height : %d\n : bpp : %d\n", sinfo.xres, sinfo.yres, sinfo.bits_per_pixel);
	printf("virtual w: %d virtual h: %d\n", sinfo.xres_virtual, sinfo.yres_virtual);

	gfx->screen_rect.x = gfx->screen_rect.y = 0;
	gfx->screen_rect.width = sinfo.xres_virtual;
	gfx->screen_rect.height = sinfo.yres_virtual;
	gfx->color_depth = sinfo.bits_per_pixel;

	set_clipping_rect(gfx->screen_rect);

	int sz = FRAMEBUFFER_SIZE(gfx->screen_rect.width, gfx->screen_rect.height, gfx->color_depth);
	gfx->framebuffer = (unsigned char*)mmap(0, sz, PROT_READ | PROT_WRITE, MAP_SHARED, gfx->dev_fd, 0);

	if(gfx->framebuffer == (void*)-1) {
		close(gfx->dev_fd);
		gfx->dev_fd = -1;
		fprintf(stderr, "Cannot map the framebuffer to memory : %s\n", strerror(errno));
		return false;
	}

// TODO: uncomment when I find how to use intelfb instead of i915 GRRRR.-	
	fb_vblank vblank;
	if(ioctl(gfx->dev_fd, FBIOGET_VBLANK, &vblank) == -1) {
//		fprintf(stderr, "FBIOGET_VBLANK error: %s\n", strerror(errno));
	}
/*	
 	else {
		printf("flags: %x\n", vblank.flags);
		printf("count: %d\n", vblank.count);
		printf("beam position: %d, %d\n", vblank.hcount, vblank.vcount);
	}
*/

	gfx->pixmap = new Pixmap;
	gfx->pixmap->width = gfx->screen_rect.width;
	gfx->pixmap->height = gfx->screen_rect.height;
	gfx->pixmap->pixels = gfx->framebuffer;

	return true;
}

void destroy_gfx()
{
	clear_screen(0, 0, 0);

	if(gfx->dev_fd != -1) {
		close(gfx->dev_fd);
	}

	gfx->dev_fd = -1;

	munmap(gfx->framebuffer, FRAMEBUFFER_SIZE(gfx->screen_rect.width, gfx->screen_rect.height, gfx->color_depth));
	gfx->framebuffer = 0;

	gfx->pixmap->pixels = 0;

	free(gfx);
}

unsigned char *get_framebuffer()
{
	return gfx->framebuffer;
}

Pixmap *get_framebuffer_pixmap()
{
	return gfx->pixmap;
}

Rect get_screen_size()
{
	return gfx->screen_rect;
}

int get_color_depth()
{
	return gfx->color_depth;
}

void set_clipping_rect(const Rect &rect)
{
	gfx->clipping_rect = rect_intersection(rect, get_screen_size());
}

const Rect &get_clipping_rect()
{
	return gfx->clipping_rect;
}

void set_cursor_visibility(bool visible)
{
	fb_cursor curs;
	curs.enable = visible ? 1 : 0;

	if(ioctl(gfx->dev_fd, FBIO_CURSOR, &curs) == -1) {
		fprintf(stderr, "Cannot toggle cursor visibility : %s\n", strerror(errno));
	}
}

void gfx_update()
{
}

void wait_vsync()
{
	unsigned long arg = 0;
	if(ioctl(gfx->dev_fd, FBIO_WAITFORVSYNC, &arg) == -1) {
//		printf("ioctl error %s\n", strerror(errno));
	}
}

#endif // WINNIE_FBDEV
