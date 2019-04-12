#ifndef __STRUCT_H__
#define __STRUCT_H__

#include <EGL/egl.h>

#ifdef __cplusplus
extern "C" {
#endif

struct surface_cb_t {
	int (* gl_prepare)(struct surface_cb_t * surface_cb, NativeWindowType window, float density);
	void (* gl_loop)(struct surface_cb_t * surface_cb);
	void * data;
};

int surface_run(struct surface_cb_t * surface_cb);

#ifdef __cplusplus
}
#endif

#endif
