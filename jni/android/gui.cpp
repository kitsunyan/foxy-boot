#include "gui.h"

using namespace android;

sp<Surface> SurfaceControl::getSurface() const { return sp<Surface>(); }

SurfaceComposerClient::SurfaceComposerClient() {}
SurfaceComposerClient::~SurfaceComposerClient() {}
sp<IBinder> SurfaceComposerClient::getBuiltInDisplay(int32_t id) { return sp<IBinder>(); }
status_t SurfaceComposerClient::getDisplayInfo(const sp<IBinder> & display, DisplayInfo * info) { return 0; }
void SurfaceComposerClient::openGlobalTransaction() {}
void SurfaceComposerClient::closeGlobalTransaction(bool synchronous) {}
