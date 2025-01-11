#ifndef SPRITE_h
#define SPRITE_h

#include <stdint.h>

typedef struct {
  uint32_t width;
  uint32_t height;
  const uint32_t *pixel_data;
} sprite_t;

void fill_rectangle(int x, int y, int width, int height, uint32_t color);
void draw_sprite(int x, int y, const sprite_t *sprite);
void draw_line(int x0, int y0, int x1, int y1, uint32_t color);

#endif /* SPRITE_h */
