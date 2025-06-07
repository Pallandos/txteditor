#include <stdio.h>
#include <string.h>
#include "minirisc.h"
#include "harvey_platform.h"
#include "uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "audio_server.h"
#include "samples.h"
#include "font.h"
#include "sprite.h"
#include "xprintf.h"
#include "sys/queue.h"

uint32_t hue_to_color(unsigned int hue);
QueueHandle_t kb_queue;

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480 

#define BLACK 0xff000000
#define WHITE 0xffffffff
#define RED   0xfff00f00
#define GREEN 0xff00ff00
#define TRANSPARENT 0x00000000

#define X_MAX 640
#define Y_MAX 480

extern const sprite_t soccer_ball;

uint32_t framebuffer[SCREEN_WIDTH*SCREEN_HEIGHT];
static SemaphoreHandle_t video_refresh_semaphore = NULL;

typedef struct {
	int x;
	int y;
} CursorArgs;

void keyboard_interrupt_handler()
{	uint32_t kdata;
	while (KEYBOARD->SR & KEYBOARD_SR_FIFO_NOT_EMPTY) {

		kdata = KEYBOARD->DATA;

		if (kdata & KEYBOARD_DATA_PRESSED) {
			xprintf("key code: %d\n", KEYBOARD_KEY_CODE(kdata));
			
			xQueueSend(kb_queue, &kdata, 0);

		}
	}
}


void video_interrupt_handler()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	VIDEO->SR &= ~VIDEO_SR_SRF_P;
	xSemaphoreGiveFromISR(video_refresh_semaphore, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void write_task(void *arg)
{
	(void)arg;

	int text_x_pos = 0;
	int text_y_pos = 0;

	
	uint32_t kdata;

	while (1) {

		xSemaphoreTake(video_refresh_semaphore, portMAX_DELAY);
		font_8x16_draw_char(text_x_pos, text_y_pos, ' ', BLACK, WHITE);

		while (xQueueReceive(kb_queue, &kdata, 0) == pdTRUE) {
			
			xSemaphoreTake(video_refresh_semaphore, portMAX_DELAY);

			// if return : go to next line
			if (KEYBOARD_KEY_CODE(kdata) == 13) {
				font_8x16_draw_char(text_x_pos, text_y_pos, ' ', WHITE, BLACK);
				text_x_pos = 0;
				if (text_y_pos >= Y_MAX - 16) {
					text_y_pos = 0;
				} else {
					text_y_pos += 16;
				}
				continue;
			}
			if (KEYBOARD_KEY_CODE(kdata) == 8) {
				// del cursor :
				font_8x16_draw_char(text_x_pos, text_y_pos, ' ', WHITE, BLACK);

				// delete the last character
				if (text_x_pos > 0) {
					text_x_pos -= 10;
				}
				font_8x16_draw_char(text_x_pos, text_y_pos, ' ', WHITE, BLACK);
				continue;
			}
			if (KEYBOARD_KEY_CODE(kdata) == 9) {
				font_8x16_draw_char(text_x_pos, text_y_pos, ' ', WHITE, BLACK);
				// tabulation
				if (text_x_pos < X_MAX - 40) {
					text_x_pos += 40;
				}
				continue;
			}
			if (KEYBOARD_KEY_CODE(kdata) == 82){
				font_8x16_draw_char(text_x_pos, text_y_pos, ' ', WHITE, BLACK);
				// up arrow
				if (text_y_pos > 0) {
					text_y_pos -= 16;
				}
				continue;
			}
			if (KEYBOARD_KEY_CODE(kdata) == 81){
				font_8x16_draw_char(text_x_pos, text_y_pos, ' ', WHITE, BLACK);
				// down arrow
				if (text_y_pos < Y_MAX - 16) {
					text_y_pos += 16;
				}
				continue;
			}

			char c = (char)KEYBOARD_KEY_CODE(kdata);
			font_8x16_draw_char(text_x_pos, text_y_pos, c, WHITE, BLACK);

			// move the cursor
			text_x_pos += 10;
		}
	}
}

void init_video()
{
	video_refresh_semaphore = xSemaphoreCreateBinary();

	VIDEO->WIDTH  = SCREEN_WIDTH;
	VIDEO->HEIGHT = SCREEN_HEIGHT;
	VIDEO->DMA_ADDR = framebuffer;
	memset(framebuffer, 0, sizeof(framebuffer));
	VIDEO->CR = VIDEO_CR_EN | VIDEO_CR_IE;
	minirisc_enable_interrupt(VIDEO_INTERRUPT);
}


int main()
{

	// init queue
	kb_queue = xQueueCreate(1024, sizeof(uint32_t)); // quid de la taille? 

	init_uart();

	init_audio_mixer(4);
	init_video();

	KEYBOARD->CR |= KEYBOARD_CR_IE;
	minirisc_enable_interrupt(KEYBOARD_INTERRUPT);

	xTaskCreate(write_task, "write", 1024, NULL, 1, NULL);

	vTaskStartScheduler();

	return 0;
}

/* Hue must be between 0 and 1536 not included */
uint32_t hue_to_color(unsigned int hue)
{
	uint32_t r, g, b;
	if (hue < 256) {
		r = 255;
		g = hue;
		b = 0;
	} else if (hue < 512) {
		r = 511 - hue;
		g = 255;
		b = 0;
	} else if (hue < 768) {
		r = 0;
		g = 255;
		b = hue - 512;
	} else if (hue < 1024) {
		r = 0;
		g = 1023 - hue;
		b = 255;
	} else if (hue < 1280) {
		r = hue - 1024;
		g = 0;
		b = 255;
	} else if (hue < 1536) {
		r = 255;
		g = 0;
		b = 1535 - hue;
	} else {
		r = 0;
		g = 0;
		b = 0;
	}
	return 0xff000000 | (r << 16) | (g << 8) | b;
}


