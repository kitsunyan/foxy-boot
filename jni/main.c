#include "font.h"
#include "source.h"
#include "surface.h"

#include <byteswap.h>
#include <stdlib.h>
#include <string.h>

#include <android/cutils.h>

#include <GLES/gl.h>
#include <GLES/glext.h>

struct surface_context_t {
	int boot_test;
};

struct source_context_t {
	EGLDisplay display;
	EGLSurface surface;
	int width;
	int height;
	int boot_test;
	int scale;
	const struct font_t * font;
	int tw;
	int th;
	uint8_t ** symbols;
	uint8_t * clear;
};

enum message_source_t {
	MESSAGE_SOURCE_KMSG,
	MESSAGE_SOURCE_LOGD,
	MESSAGE_SOURCE_UNKNOWN
};

static uint8_t * make_symbol(const struct font_t * font, uint8_t c, int scale,
	uint32_t background_color, uint32_t foreground_color) {
	int xwb = FONT_WIDTH_BYTES(font->width);
	const uint8_t * cd = &font->data[xwb * font->height * c];
	int total = font->width * font->height * scale * scale;
	uint8_t * rgba = malloc(4 * total);
	uint32_t * pixels = (uint32_t *) rgba;
	int x, y, sx, sy;
	for (y = 0; y < font->height; y++) {
		for (x = 0; x < font->width; x++) {
			int dot = (cd[xwb * y + (x / 8)] >> (8 - x % 8)) & 1;
			int line = font->width * scale;
			int base = y * scale * line + x * scale;
			for (sy = 0; sy < scale; sy++) {
				for (sx = 0; sx < scale; sx++) {
					pixels[base + sx + sy * line] = bswap_32(dot ? foreground_color : background_color);
				}
			}
		}
	}
	return rgba;
}

static int check_exit(int boot_test) {
	char value[PROPERTY_VALUE_MAX];
	property_get("service.bootanim.exit", value, "0");
	if (atoi(value) && !boot_test) {
		return 1;
	}
	property_get("init.svc.surfaceflinger", value, "stopped");
	if (strcmp(value, "running")) {
		return 1;
	}
	return 0;
}

static void render_line(int line, const char * text, const struct font_t * font, uint8_t ** symbols, int scale) {
	int length = strlen(text);
	int i;
	for (i = 0; i < length; i++) {
		glTexSubImage2D(GL_TEXTURE_2D, 0, i * font->width * scale, line * font->height * scale,
			font->width * scale, font->height * scale, GL_RGBA, GL_UNSIGNED_BYTE, symbols[(int) text[i]]);
	}
}

static void source_callback(struct ring_t * ring, void * data) {
	struct source_context_t * context = data;
	int i, j, k;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, context->tw, context->th, 0, GL_RGBA, GL_UNSIGNED_BYTE, context->clear);
	for (i = ring->start, j = 0; i < ring->start + ring->total; i++, j++) {
		k = i >= ring->height ? i % ring->height : i;
		render_line(j, &ring->lines[k * ring->width], context->font, context->symbols, context->scale);
	}
	glDrawTexiOES(0, 0, 0, context->width, context->height);
	eglSwapBuffers(context->display, context->surface);
}

static int source_check_exit(void * data) {
	struct source_context_t * context = data;
	return check_exit(context->boot_test);
}

static void loop(EGLDisplay display, EGLSurface surface, int width, int height,
	int boot_test, int scale, uint32_t background_color, uint32_t foreground_color,
	const struct font_t * font, enum message_source_t message_source) {
	GLint crop[4] = { 0, height, width, -height };
	struct ring_t ring;
	struct source_context_t context = {
		.display = display,
		.surface = surface,
		.width = width,
		.height = height,
		.boot_test = boot_test,
		.scale = scale,
		.font = font
	};
	int i;

	context.tw = 1 << (31 - __builtin_clz(width));
	context.th = 1 << (31 - __builtin_clz(height));
	if (context.tw < width) {
		context.tw <<= 1;
	}
	if (context.th < height) {
		context.th <<= 1;
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glShadeModel(GL_FLAT);
	glDisable(GL_DITHER);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_BLEND);

	glBindTexture(GL_TEXTURE_2D, 0);
	glEnable(GL_TEXTURE_2D);
	glTexEnvx(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_CROP_RECT_OES, crop);

	context.symbols = malloc(sizeof(context.symbols) * font->count);
	for (i = 0; i < font->count; i++) {
		context.symbols[i] = make_symbol(font, i, scale,
			background_color, foreground_color);
	}
	context.clear = malloc(4 * context.tw * context.th);
	for (i = 0; i < context.tw * context.th; i++) {
		((uint32_t *) context.clear)[i] = bswap_32(background_color);
	}

	ring.width = width / font->width / scale + 1;
	ring.height = height / font->height / scale;
	ring.total = 0;
	ring.start = 0;
	ring.lines = malloc(ring.width * ring.height);

	switch (message_source) {
		case MESSAGE_SOURCE_KMSG: {
			source_kmsg(&ring, &context, source_callback, source_check_exit);
			break;
		}
		case MESSAGE_SOURCE_LOGD: {
			source_logd(&ring, &context, source_callback, source_check_exit);
			break;
		}
		default: {
			source_unknown(&ring, &context, source_callback, source_check_exit);
			break;
		}
	}

	free(ring.lines);
	free(context.clear);
	for (i = 0; i < font->count; i++) {
		free(context.symbols[i]);
	}
	free(context.symbols);
}

static int surface_callback(struct surface_cb_t * surface_cb, NativeWindowType window, float density) {
	struct surface_context_t * context = surface_cb->data;
	char value[PROPERTY_VALUE_MAX];
	int ivalue;
	const EGLint attribs[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_DEPTH_SIZE, 0,
		EGL_NONE
	};
	EGLint width, height;
	EGLint numConfigs;
	EGLConfig config;
	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	EGLSurface surface;
	EGLContext eglcontext;
	int scale;
	uint32_t background_color;
	uint32_t foreground_color;
	const struct font_t * font;
	enum message_source_t message_source;

	eglInitialize(display, 0, 0);
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);
	surface = eglCreateWindowSurface(display, config, window, NULL);
	eglcontext = eglCreateContext(display, config, NULL, NULL);
	eglQuerySurface(display, surface, EGL_WIDTH, &width);
	eglQuerySurface(display, surface, EGL_HEIGHT, &height);

	if (eglMakeCurrent(display, surface, surface, eglcontext) == EGL_FALSE) {
		return 0;
	}

	property_get("foxy.boot.scale", value, "0");
	ivalue = atoi(value);
	scale = ivalue > 0 && ivalue < 10 ? ivalue : (int) density;

	property_get("foxy.boot.background", value, "#000000");
	background_color = value[0] == '#' && strlen(value) == 7
		? strtol(&value[1], NULL, 16) << 8 | 0xff : 0x000000ff;

	property_get("foxy.boot.foreground", value, "#ffffff");
	foreground_color = value[0] == '#' && strlen(value) == 7
		? strtol(&value[1], NULL, 16) << 8 | 0xff : 0xffffffff;

	property_get("foxy.boot.source", value, "kmsg");
	if (!strcmp(value, "kmsg")) {
		message_source = MESSAGE_SOURCE_KMSG;
	} else if (!strcmp(value, "logd")) {
		message_source = MESSAGE_SOURCE_LOGD;
	} else {
		message_source = MESSAGE_SOURCE_UNKNOWN;
	}

	font = get_font();

	loop(display, surface, width, height,
		context->boot_test, scale, background_color, foreground_color,
		font, message_source);

	eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(display, eglcontext);
	eglDestroySurface(display, surface);
	eglTerminate(display);
	return 1;
}

int main(int argc, char ** argv) {
	if (argc >= 2 && !strcmp(argv[1], "ldcheck")) {
		return surface_run(NULL);
	} else {
		char value[PROPERTY_VALUE_MAX];
		property_get("debug.sf.nobootanimation", value, "0");
		if (!atoi(value)) {
			struct surface_context_t context;
			struct surface_cb_t surface_cb = {
				.callback = surface_callback,
				.data = &context
			};
			context.boot_test = argc >= 2 && !strcmp(argv[1], "test");
			return surface_run(&surface_cb);
		}
		return 0;
	}
}
