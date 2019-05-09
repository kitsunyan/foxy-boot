#include "source.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

struct log_entry {
	uint16_t len;
	uint16_t hdr_size;
	int32_t pid;
	int32_t tid;
	uint32_t sec;
	uint32_t nsec;
};

static void time_diff(struct timespec * target, struct timespec * what) {
	if (target->tv_sec > what->tv_sec ||
		(target->tv_sec == what->tv_sec && target->tv_nsec >= what->tv_nsec)) {
		target->tv_sec -= what->tv_sec;
		if (target->tv_nsec >= what->tv_nsec) {
			target->tv_nsec -= what->tv_nsec;
		} else {
			target->tv_sec--;
			target->tv_nsec += 1000000000 - what->tv_nsec;
		}
	} else {
		target->tv_sec = 0;
		target->tv_nsec = 0;
	}
}

static void obtain_time_prefix(struct log_entry * entry, char * time) {
	struct timespec realtime;
	struct timespec monotonic;
	struct timespec log = {
		.tv_sec = entry->sec,
		.tv_nsec = entry->nsec
	};
	clock_gettime(CLOCK_REALTIME, &realtime);
	clock_gettime(CLOCK_MONOTONIC, &monotonic);
	time_diff(&realtime, &monotonic);
	time_diff(&log, &realtime);
	sprintf(time, "[%12.6f] ", log.tv_sec + log.tv_nsec / 1000000000.);
}

void source_logd(struct ring_t * ring, void * data,
	void (* callback)(struct ring_t *, void *), int (* check_exit)(void *)) {
	struct sockaddr_un addr = {
		.sun_family = AF_UNIX,
		.sun_path = "/dev/socket/logdr"
	};
	int force_call = 1;
	int fd = -1;
	char buffer[5 * 1024];
	struct log_entry * entry = (void *) buffer;

	while (1) {
		int call = force_call;
		force_call = 0;

		if (fd < 0) {
			fd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK, 0);
			if (fd >= 0 && connect(fd, (struct sockaddr *) &addr, sizeof(addr))) {
				close(fd);
				fd = -1;
			}
			if (fd >= 0) {
				static const char * msg = "stream lids=0,3,4";
				int len = strlen(msg);
				if (write(fd, msg, len - 1) != len - 1) {
					close(fd);
					fd = -1;
				}
			}
		}

		if (fd >= 0) {
			while (1) {
				int i, start, from;
				char time[30];
				int time_size;
				int size = recv(fd, buffer, sizeof(buffer), 0);

				if (size < (int) sizeof(*entry)) {
					if (errno != EAGAIN && errno != EWOULDBLOCK) {
						close(fd);
						fd = -1;
					}
					break;
				}
				if (entry->len + entry->hdr_size != size) {
					close(fd);
					fd = -1;
					break;
				}

				while (size > 0 && buffer[size - 1] <= ' ') {
					size--;
				}
				for (i = entry->hdr_size; i < size; i++) {
					if (!buffer[i] || buffer[i] == '\t') {
						buffer[i] = ' ';
					}
				}
				call = 1;
				start = entry->hdr_size;
				if (buffer[start] <= ' ') {
					start++;
				}
				from = start;
				obtain_time_prefix(entry, time);
				time_size = strlen(time);

				for (i = start; i < size; i++) {
					int new_line = buffer[i] == '\n';
					if (i - from == ring->width - 1 - time_size - 1 || new_line || i == size - 1) {
						char * target = ring_current(ring);
						int copy_count = i - from + (new_line ? 0 : 1);
						if (copy_count > 0) {
							memcpy(target, time, time_size);
							memcpy(&target[time_size], &buffer[from], copy_count);
							target[time_size + copy_count] = '\0';
							ring_increment(ring);
						}
						from = i + 1;
						time_size = new_line ? strlen(time) : 0;
					}
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

	if (fd >= 0) {
		close(fd);
	}
}
