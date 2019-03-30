#ifndef __FONT_H__
#define __FONT_H__

#include <stdint.h>

#define FONT_WIDTH_BYTES(width) (((width) + 7) / 8)

struct font_t {
	int width;
	int height;
	int count;
	const uint8_t * data;
};

const struct font_t * get_font();

#endif
