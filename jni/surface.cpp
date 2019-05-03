#include "surface.h"

#include <dlfcn.h>
#include <sys/resource.h>
#include <android/gui.h>
#include <stdio.h>

typedef android::sp<android::SurfaceControl> (* create_surface_t)(android::SurfaceComposerClient *,
	android::String8 const &, uint32_t, uint32_t, int32_t, uint32_t, android::SurfaceControl *, uint32_t, uint32_t);
typedef void (* set_layer_t)(android::SurfaceControl *, uint32_t);
typedef void (* open_global_transaction_t)();
typedef void (* close_global_transaction_t)(bool);

static void * find_symbol(const char ** symbols) {
	while (*symbols) {
		void * symbol = dlsym(RTLD_NEXT, *symbols);
		if (symbol) {
			return symbol;
		}
		symbols++;
	}
	return nullptr;
}

#define SYMBOL_LOOKUP(name, ...) \
	static const char * name##_symbols[] = { __VA_ARGS__, nullptr }; \
	name##_t name = reinterpret_cast<name##_t>(find_symbol(name##_symbols));

static void print_symbol_error(const char * name) {
	fprintf(stderr, "\"%s\" is not available\n", name);
}

int surface_run(surface_cb_t * surface_cb) {
	SYMBOL_LOOKUP(create_surface,
		"_ZN7android21SurfaceComposerClient13createSurfaceERKNS_7String8EjjijPNS_14SurfaceControlEjj",
		"_ZN7android21SurfaceComposerClient13createSurfaceERKNS_7String8EjjijPNS_14SurfaceControlEii",
		"_ZN7android21SurfaceComposerClient13createSurfaceERKNS_7String8Ejjij");
	if (!create_surface) {
		print_symbol_error("android::SurfaceComposerClient::createSurface");
		return 1;
	}

	SYMBOL_LOOKUP(set_layer,
		"_ZN7android14SurfaceControl8setLayerEj",
		"_ZN7android14SurfaceControl8setLayerEi");
	SYMBOL_LOOKUP(open_global_transaction,
		"_ZN7android21SurfaceComposerClient21openGlobalTransactionEv");
	SYMBOL_LOOKUP(close_global_transaction,
		"_ZN7android21SurfaceComposerClient22closeGlobalTransactionEb");

	if ((set_layer || open_global_transaction || close_global_transaction) &&
		(!set_layer || !open_global_transaction || !close_global_transaction)) {
		if (!set_layer) {
			print_symbol_error("android::SurfaceControl::setLayer");
		}
		if (!open_global_transaction) {
			print_symbol_error("android::SurfaceComposerClient::openGlobalTransaction");
		}
		if (!close_global_transaction) {
			print_symbol_error("android::SurfaceComposerClient::closeGlobalTransaction");
		}
		return 1;
	}

	if (!surface_cb) {
		return 0;
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

		if (set_layer) {
			open_global_transaction();
			set_layer(control, 0x40000000);
			close_global_transaction(false);
		}

		android::Surface * surface = control->getSurface().get();
		if (surface_cb->callback(surface_cb, surface, dinfo.density)) {
			success = true;
		}
	}
	client->decStrong(&ref);
	setpriority(PRIO_PROCESS, 0, android::PRIORITY_DEFAULT);
	return success ? 0 : 1;
}
