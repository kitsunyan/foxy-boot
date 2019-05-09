#ifndef __SOURCE_H__
#define __SOURCE_H__

struct ring_t {
	int width;
	int height;
	int total;
	int start;
	char * lines;
};

char * ring_current(struct ring_t * ring);
void ring_increment(struct ring_t * ring);

void source_kmsg(struct ring_t * ring, void * data,
	void (* callback)(struct ring_t *, void *), int (* check_exit)(void *));

void source_logd(struct ring_t * ring, void * data,
	void (* callback)(struct ring_t *, void *), int (* check_exit)(void *));

void source_unknown(struct ring_t * ring, void * data,
	void (* callback)(struct ring_t *, void *), int (* check_exit)(void *));

#endif
