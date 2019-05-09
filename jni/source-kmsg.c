#include "source.h"

#include <stdlib.h>
#include <string.h>
#include <sys/klog.h>
#include <unistd.h>

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

void source_kmsg(struct ring_t * ring, void * data,
	void (* callback)(struct ring_t *, void *), int (* check_exit)(void *)) {
	int force_call = 1;
	int log_size;
	char * log;
	char last_date[30] = "";
	int i;

	log_size = klogctl(10, NULL, 0);
	if (log_size < 16 * 1024) {
		log_size = 16 * 1024;
	}
	log = malloc(log_size);

	while (1) {
		int count = klogctl(3, log, log_size);
		int call;
		i = count > 0 ? next_after_date_update(last_date, log, count) : -1;
		call = i >= 0 || force_call;
		force_call = 0;

		if (i >= 0) {
			int new_line = 1;
			while (i < count) {
				int start;
				int copy_count;
				if (new_line && log[i] == '<') {
					while (log[i++] != '>' && i < count);
				}
				start = i;
				while (log[i++] != '\n' && i < count && i - start < ring->width - 1);
				new_line = i > 0 && log[i - 1] == '\n';
				copy_count = i - start - (new_line ? 1 : 0);
				if (copy_count > 0) {
					char * target = ring_current(ring);
					memcpy(target, &log[start], copy_count);
					target[copy_count] = '\0';
					ring_increment(ring);
				}
			}
		}

		if (call) {
			callback(ring, data);
		}
		usleep(20000);
		if (check_exit(data)) {
			break;
		}
	}

	free(log);
}
