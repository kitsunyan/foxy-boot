#ifndef __STRUCT_H__
#define __STRUCT_H__

#include <EGL/egl.h>

#ifdef __cplusplus
extern "C" {
#endif

struct surface_cb_t {
	int (* callback)(struct surface_cb_t * surface_cb, NativeWindowType window, float density);
	void * data;
};

int surface_run(struct surface_cb_t * surface_cb);

#ifdef __cplusplus
}
#endif

#endif
