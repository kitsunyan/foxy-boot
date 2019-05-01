#include "font.h"
#include "surface.h"

#include <dlfcn.h>

#include <byteswap.h>
#include <sys/klog.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <android/cutils.h>

#include <GLES/gl.h>
#include <GLES/glext.h>

struct ctx_t {
	const struct font_t * font;
	int boot_test;

	int width;
	int height;
	int scale;
	uint32_t background_color;
	uint32_t foreground_color;

	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
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

static int check_exit(struct ctx_t * ctx) {
	char value[PROPERTY_VALUE_MAX];
	property_get("service.bootanim.exit", value, "0");
	if (atoi(value) && !ctx->boot_test) {
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

static int next_after_date_update(char * date, const char * log, int count) {
	int new_date = -1;
	int lastl = -1;
	int lastc = -1;
	int datel = strlen(date);
	int i;
	for (i = count - 1; i >= 0; i--) {
		if (log[i] == ']') {
			lastc = i + 1;
		} else if ((log[i] == '<' || log[i] == '[') && (i == 0 || log[i - 1] == '\n') && lastc > i) {
			if (lastc - i == datel && !strncmp(&log[i], date, datel)) {
				break;
			} else {
				lastl = i;
				if (new_date < 0) {
					new_date = i;
				}
			}
		}
	}
	if (new_date >= 0) {
		if (datel == 0) {
			lastl = -1;
		}
		i = new_date;
		while (log[i++] != ']');
		memcpy(date, &log[new_date], i - new_date);
		date[i - new_date] = '\0';
	}
	return lastl;
}

static void loop(struct surface_cb_t * surface_cb) {
	struct ctx_t * ctx = surface_cb->data;
	GLint crop[4] = { 0, ctx->height, ctx->width, -ctx->height };
	uint8_t * symbols[ctx->font->count];
	uint8_t * clear;
	int log_size;
	char * log, * lines;
	int i, max_line, max_lines;
	int ring_start = 0;
	int ring_total = 0;
	int force_render = 1;
	char last_date[30] = "";

	int tw = 1 << (31 - __builtin_clz(ctx->width));
	int th = 1 << (31 - __builtin_clz(ctx->height));
	if (tw < ctx->width) {
		tw <<= 1;
	}
	if (th < ctx->height) {
		th <<= 1;
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

	for (i = 0; i < ctx->font->count; i++) {
		symbols[i] = make_symbol(ctx->font, i, ctx->scale,
			ctx->background_color, ctx->foreground_color);
	}
	clear = malloc(4 * tw * th);
	for (i = 0; i < tw * th; i++) {
		((uint32_t *) clear)[i] = bswap_32(ctx->background_color);
	}
	log_size = klogctl(10, NULL, 0);
	if (log_size < 16 * 1024) {
		log_size = 16 * 1024;
	}
	log = malloc(log_size);
	max_line = ctx->width / ctx->font->width / ctx->scale;
	max_lines = ctx->height / ctx->font->height / ctx->scale;
	lines = malloc(max_lines * (max_line + 1));

	while (1) {
		int count = klogctl(3, log, log_size);
		int render;
		i = count > 0 ? next_after_date_update(last_date, log, count) : -1;
		render = i >= 0 || force_render;
		force_render = 0;
		if (i >= 0) {
			int new_line = 1;
			while (i < count) {
				int start;
				int copy_count;
				if (new_line && log[i] == '<') {
					while (log[i++] != '>' && i < count);
				}
				start = i;
				while (log[i++] != '\n' && i < count && i - start < max_line);
				new_line = i > 0 && log[i - 1] == '\n';
				copy_count = i - start - (new_line ? 1 : 0);
				if (copy_count > 0) {
					char * target = &lines[(ring_total < max_lines ? ring_total : ring_start) * (max_line + 1)];
					memcpy(target, &log[start], copy_count);
					target[copy_count] = '\0';
					if (ring_total >= max_lines) {
						ring_start = (ring_start + 1) % max_lines;
					} else {
						ring_total++;
					}
				}
			}
		}
		if (render) {
			int j;
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0, GL_RGBA, GL_UNSIGNED_BYTE, clear);
			for (i = ring_start, j = 0; i < ring_start + ring_total; i++, j++) {
				int k = i >= max_lines ? i % max_lines : i;
				render_line(j, &lines[k * (max_line + 1)], ctx->font, symbols, ctx->scale);
			}
			glDrawTexiOES(0, 0, 0, ctx->width, ctx->height);
			eglSwapBuffers(ctx->display, ctx->surface);
		}
		usleep(20000);
		if (check_exit(ctx)) {
			break;
		}
	}

	free(clear);
	free(log);
	free(lines);
	for (i = 0; i < ctx->font->count; i++) {
		free(symbols[i]);
	}

	eglMakeCurrent(ctx->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(ctx->display, ctx->context);
	eglDestroySurface(ctx->display, ctx->surface);
	eglTerminate(ctx->display);
}

static int prepare(struct surface_cb_t * surface_cb, NativeWindowType window, float density) {
	struct ctx_t * ctx = surface_cb->data;
	char value[PROPERTY_VALUE_MAX];
	int ivalue;
	const EGLint attribs[] = {
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_DEPTH_SIZE, 0,
		EGL_NONE
	};
	EGLint w, h;
	EGLint numConfigs;
	EGLConfig config;
	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	EGLSurface surface;
	EGLContext context;

	eglInitialize(display, 0, 0);
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);
	surface = eglCreateWindowSurface(display, config, window, NULL);
	context = eglCreateContext(display, config, NULL, NULL);
	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
		return 0;
	}

	ctx->width = w;
	ctx->height = h;

	property_get("foxy.boot.scale", value, "0");
	ivalue = atoi(value);
	ctx->scale = ivalue > 0 && ivalue < 10 ? ivalue : (int) density;

	property_get("foxy.boot.background", value, "#000000");
	ctx->background_color = value[0] == '#' && strlen(value) == 7
		? strtol(&value[1], NULL, 16) << 8 | 0xff : 0x000000ff;

	property_get("foxy.boot.foreground", value, "#ffffff");
	ctx->foreground_color = value[0] == '#' && strlen(value) == 7
		? strtol(&value[1], NULL, 16) << 8 | 0xff : 0xffffffff;

	ctx->display = display;
	ctx->surface = surface;
	ctx->context = context;
	return 1;
}

int main(int argc, char ** argv) {
	if (argc >= 2 && !strcmp(argv[1], "ldcheck")) {
		return surface_run(NULL);
	} else {
		char value[PROPERTY_VALUE_MAX];
		property_get("debug.sf.nobootanimation", value, "0");
		if (!atoi(value)) {
			struct ctx_t ctx;
			struct surface_cb_t surface_cb = {
				.gl_prepare = prepare,
				.gl_loop = loop,
				.data = &ctx
			};
			ctx.font = get_font();
			ctx.boot_test = argc >= 2 && !strcmp(argv[1], "test");
			return surface_run(&surface_cb);
		}
		return 0;
	}
}
