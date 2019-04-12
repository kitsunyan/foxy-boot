#include "surface.h"

#include <dlfcn.h>
#include <sys/resource.h>
#include <android/gui.h>
#include <stdio.h>

typedef void (* set_layer_t)(android::SurfaceControl *, uint32_t);
typedef android::sp<android::SurfaceControl> (* create_surface_t)(android::SurfaceComposerClient *,
	android::String8 const &, uint32_t, uint32_t, int32_t, uint32_t, android::SurfaceControl *, uint32_t, uint32_t);

static void * find_symbol(void * object, const char * name, const char ** symbols) {
	while (*symbols) {
		void * symbol = dlsym(object, *symbols);
		if (symbol) {
			return symbol;
		}
		symbols++;
	}
	fprintf(stderr, "\"%s\" is not available\n", name);
	return nullptr;
}

int surface_run(surface_cb_t * surface_cb) {
	void * gui = dlopen("libgui.so", RTLD_LAZY);
	if (!gui) {
		fprintf(stderr, "dlopen failed\n");
		return 1;
	}
	const char * set_layer_symbols[3] = {
		"_ZN7android14SurfaceControl8setLayerEj",
		"_ZN7android14SurfaceControl8setLayerEi",
		nullptr
	};
	set_layer_t set_layer = reinterpret_cast<set_layer_t>(find_symbol(gui,
		"android::SurfaceControl::setLayer", set_layer_symbols));
	const char * create_surface_symbols[3] = {
		"_ZN7android21SurfaceComposerClient13createSurfaceERKNS_7String8EjjijPNS_14SurfaceControlEjj",
		"_ZN7android21SurfaceComposerClient13createSurfaceERKNS_7String8Ejjij",
		nullptr
	};
	create_surface_t create_surface = reinterpret_cast<create_surface_t>(find_symbol(gui,
		"android::SurfaceComposerClient::createSurface", create_surface_symbols));
	if (!set_layer || !create_surface) {
		return 1;
	}

	int ref = 0;
	setpriority(PRIO_PROCESS, 0, android::PRIORITY_DISPLAY);
	android::SurfaceComposerClient * client = new android::SurfaceComposerClient();
	client->incStrong(&ref);
	android::sp<android::IBinder> dtoken(android::SurfaceComposerClient::getBuiltInDisplay(0));
	android::DisplayInfo dinfo;
	android::status_t status = android::SurfaceComposerClient::getDisplayInfo(dtoken, &dinfo);
	bool success = false;
	if (status == android::NO_ERROR) {
		android::SurfaceControl * control = create_surface(client,
			android::String8("SurfaceThread"), dinfo.w, dinfo.h, 1, 0, nullptr, -1, -1).get();

		android::SurfaceComposerClient::openGlobalTransaction();
		set_layer(control, 0x40000000);
		android::SurfaceComposerClient::closeGlobalTransaction();

		android::Surface * surface = control->getSurface().get();
		if (surface_cb->gl_prepare(surface_cb, surface, dinfo.density)) {
			surface_cb->gl_loop(surface_cb);
			success = true;
		}
	}
	client->decStrong(&ref);
	setpriority(PRIO_PROCESS, 0, android::PRIORITY_DEFAULT);
	return success ? 0 : 1;
}
