#include "source.h"

#include <string.h>
#include <unistd.h>

char * ring_current(struct ring_t * ring) {
	return &ring->lines[(ring->total < ring->height ? ring->total : ring->start) * ring->width];
}

void ring_increment(struct ring_t * ring) {
	if (ring->total >= ring->height) {
		ring->start = (ring->start + 1) % ring->height;
	} else {
		ring->total++;
	}
}

void source_unknown(struct ring_t * ring, void * data,
	void (* callback)(struct ring_t *, void *), int (* check_exit)(void *)) {
	static const char * text = "Unknown message source!";
	int text_length = strlen(text);
	int i;

	for (i = 0; i < (text_length + ring->width - 2) / (ring->width - 1); i++) {
		char * target = ring_current(ring);
		int copy_from = i * (ring->width - 1);
		int copy_count = text_length - copy_from;
		if (copy_count > ring->width - 1) {
			copy_count = ring->width - 1;
		}
		memcpy(target, &text[copy_from], copy_count);
		target[copy_count] = '\0';
		ring_increment(ring);
	}
	callback(ring, data);

	while (!check_exit(data)) {
		usleep(20000);
	}
}
