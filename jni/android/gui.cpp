#include "gui.h"

using namespace android;

status_t SurfaceControl::setLayer(uint32_t layer) { return 0; }
sp<Surface> SurfaceControl::getSurface() const { return nullptr; }

static int stub = 0;
SurfaceComposerClient::SurfaceComposerClient(): mComposer(stub) {}
SurfaceComposerClient::~SurfaceComposerClient() {}
status_t SurfaceComposerClient::linkToComposerDeath(const sp<IBinder::DeathRecipient> & recipient,
	void * cookie, uint32_t flags) { return 0; }
sp<SurfaceControl> SurfaceComposerClient::createSurface(const String8 & name, uint32_t w, uint32_t h,
	PixelFormat format, uint32_t flags) { return nullptr; }
sp<IBinder> SurfaceComposerClient::getBuiltInDisplay(int32_t id) { return nullptr; }
status_t SurfaceComposerClient::getDisplayInfo(const sp<IBinder> & display, DisplayInfo * info) { return 0; }
void SurfaceComposerClient::openGlobalTransaction() {}
void SurfaceComposerClient::closeGlobalTransaction(bool synchronous) {}
