#include "harvey_platform.h"
#include "sprite.h"


void fill_rectangle(int x, int y, int width, int height, uint32_t color)
{
	const int screen_width  = VIDEO->WIDTH;
	const int screen_height = VIDEO->HEIGHT;
	uint32_t *framebuffer   = (uint32_t*)VIDEO->DMA_ADDR;
	if (x >= screen_width || y >= screen_width || (x + width) <= 0 || (y + height) <= 0)
		return;
	int ystart =  y >= 0 ? y : 0;
	int ystop  = (y + height) < screen_height ? (y + height) : screen_height;
	int xstart =  x >= 0 ? x : 0;
	int xstop  = (x + width) < screen_width ? (x + width) : screen_width;
	for (int y = ystart; y < ystop; y++) {
		for (int x = xstart; x < xstop; x++) {
			framebuffer[y*screen_width+x] = color;
		}
	}
}


void draw_sprite(int x, int y, const sprite_t *sprite)
{
	const int screen_width  = VIDEO->WIDTH;
	const int screen_height = VIDEO->HEIGHT;
	uint32_t *framebuffer   = (uint32_t*)VIDEO->DMA_ADDR;
	if (!sprite)
		return;
	int width  = sprite->width; 
	int height = sprite->height; 
	const uint32_t *data = sprite->pixel_data;
	if (x >= screen_width || y >= screen_width || (x + width) <= 0 || (y + height) <= 0)
		return;

	int ystart = y;
	int yc = 0;
	if (y < 0) {
		ystart = 0;
		yc = -y;
	}
	int ystop = (y + height) <= screen_height ? (y + height) : screen_height;

	int xstart = x;
	int xc = 0;
	if (x < 0) {
		xstart = 0;
		xc = -x;
	}
	int xstop = (x + width) < screen_width ? (x + width) : screen_width;

	data = &data[yc*width];
	for (int y = ystart; y < ystop; y++, data += width) {
		int p = xc;
		for (int x = xstart; x < xstop; x++) {
			uint32_t new_color = data[p++];
			uint32_t alpha = new_color >> 24;
			if (alpha == 255) {
				framebuffer[y*screen_width+x] = new_color;
			} else if (alpha != 0) {
				if (alpha > 0) alpha += 1;
				uint32_t one_minus_alpha = 256 - alpha;
				uint32_t previous_color = framebuffer[y*screen_width+x];
				uint32_t r = ((((previous_color >> 16) & 0xff) * one_minus_alpha) >> 8) + ((((new_color >> 16) & 0xff) * alpha) >> 8);
				uint32_t g = ((((previous_color >>  8) & 0xff) * one_minus_alpha) >> 8) + ((((new_color >>  8) & 0xff) * alpha) >> 8);
				uint32_t b = ((((previous_color >>  0) & 0xff) * one_minus_alpha) >> 8) + ((((new_color >>  0) & 0xff) * alpha) >> 8);
				framebuffer[y*screen_width+x] = (r << 16) | (g << 8) | b;
			}
		}
	}
}


static void draw_line_horizontal(int x, int y, int length, uint32_t color)
{
	const int screen_width  = VIDEO->WIDTH;
	const int screen_height = VIDEO->HEIGHT;
	uint32_t *framebuffer   = (uint32_t*)VIDEO->DMA_ADDR;
	if (length < 0) {
		x += length;
		length = -length;
	}
	if (y < 0 || y >= screen_height || x >= screen_width || (x + length) <= 0)
		return;
	int xstart = x >= 0 ? x : 0;
	int xstop = (x + length) <= screen_width ? (x + length) : screen_width;
	for (x = xstart; x < xstop; x++)
		framebuffer[y*screen_width+x] = color;
}


static void draw_line_vertical(int x, int y, int length, uint32_t color)
{
	const int screen_width  = VIDEO->WIDTH;
	const int screen_height = VIDEO->HEIGHT;
	uint32_t *framebuffer   = (uint32_t*)VIDEO->DMA_ADDR;
	if (length < 0) {
		y += length;
		length = -length;
	}
	if (x < 0 || x >= screen_width || y >= screen_height || (y + length) <= 0)
		return;
	int ystart = y >= 0 ? y : 0;
	int ystop = (y + length) <= screen_height ? (y + length) : screen_height;
	for (y = ystart; y < ystop; y++)
		framebuffer[y*screen_width+x] = color;
}


static void bresenham_low(int x0, int y0, int x1, int y1, uint32_t color)
{
	const int screen_width  = VIDEO->WIDTH;
	const int screen_height = VIDEO->HEIGHT;
	uint32_t *framebuffer   = (uint32_t*)VIDEO->DMA_ADDR;
	int dx = x1 - x0;
	int dy = y1 - y0;
	int yi = 1;
	if (dy < 0) {
		yi = -1;
		dy = -dy;
	}
	int d = 2*dy - dx;
	int y = y0;
	for (int x = x0; x <= x1; x++) {
		if (x >= 0 && x < screen_width && y >= 0 && y < screen_height) {
			framebuffer[y*screen_width+x] = color;
		}
		if (d > 0) {
			y += yi;
			d += 2*(dy - dx);
		} else {
			d += 2*dy;
		}
	}
}


static void bresenham_high(int x0, int y0, int x1, int y1, uint32_t color)
{
	const int screen_width  = VIDEO->WIDTH;
	const int screen_height = VIDEO->HEIGHT;
	uint32_t *framebuffer   = (uint32_t*)VIDEO->DMA_ADDR;
	int dx = x1 - x0;
	int dy = y1 - y0;
	int xi = 1;
	if (dx < 0) {
		xi = -1;
		dx = -dx;
	}
	int d = 2*dx - dy;
	int x = x0;
	for (int y = y0; y <= y1; y++) {
		if (x >= 0 && x < screen_width && y >= 0 && y < screen_height) {
			framebuffer[y*screen_width+x] = color;
		}
		if (d > 0) {
			x += xi;
			d += 2*(dx - dy);
		} else {
			d += 2*dx;
		}
	}
}


void draw_line(int x0, int y0, int x1, int y1, uint32_t color)
{
	int absy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
	int absx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
	if (absy == 0) {
		draw_line_horizontal(x0, y0, x1 - x0, color);
	} else if (absx == 0) {
		draw_line_vertical(x0, y0, y1 - y0, color);
	} else if (absy < absx) {
		if (x0 > x1) {
			bresenham_low(x1, y1, x0, y0, color);
		} else {
			bresenham_low(x0, y0, x1, y1, color);
		}
	} else {
		if (y0 > y1) {
			bresenham_high(x1, y1, x0, y0, color);
		} else {
			bresenham_high(x0, y0, x1, y1, color);
		}
	}
}


