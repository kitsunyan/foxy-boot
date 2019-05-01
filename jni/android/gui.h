#ifndef __ANDROID_GUI__
#define __ANDROID_GUI__

#include <android/utils.h>

struct ANativeWindow {
	int dummy;
};

namespace android {

class Surface: public ANativeWindow, public RefBase {};

class SurfaceControl: public RefBase {
public:
	sp<Surface> getSurface() const;
};

struct DisplayInfo {
	uint32_t w;
	uint32_t h;
	float xdpi;
	float ydpi;
	float fps;
	float density;
	uint8_t orientation;
	bool secure;
	int64_t appVsyncOffset;
	int64_t presentationDeadline;
};

class IBinder: public RefBase {};

class SurfaceComposerClient: public RefBase {
public:
	SurfaceComposerClient();
	virtual ~SurfaceComposerClient();

	static sp<IBinder> getBuiltInDisplay(int32_t id);
	static status_t getDisplayInfo(const sp<IBinder> & display, DisplayInfo * info);

private:
	char reserved[64];
};

}

#endif
