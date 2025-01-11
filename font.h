#ifndef FONT_H
#define FONT_H

#include <stdint.h>

void font_8x16_draw_char(int x, int y, char c, uint32_t color_on, uint32_t color_off);
int  font_8x16_draw_text(int x, int y, const char *str, uint32_t color_on, uint32_t color_off);

void font_16x32_draw_char(int x, int y, char c, uint32_t color_on, uint32_t color_off);
int  font_16x32_draw_text(int x, int y, const char *str, uint32_t color_on, uint32_t color_off);

#endif /* FONT_H */
