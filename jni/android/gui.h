#ifndef __ANDROID_GUI__
#define __ANDROID_GUI__

#include <android/binder.h>
#include <android/utils.h>

struct ANativeWindow {
	int dummy;
};

namespace android {

struct ISurfaceComposer {
	enum {
		eDisplayIdMain = 0
	};
};

class Surface: public ANativeWindow, public RefBase {};

class SurfaceControl: public RefBase {
public:
	status_t setLayer(uint32_t layer);
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

typedef int32_t PixelFormat;

enum {
	PIXEL_FORMAT_RGBA_8888 = 1
};

class SurfaceComposerClient: public RefBase {
public:
	SurfaceComposerClient();
	virtual ~SurfaceComposerClient();

	status_t linkToComposerDeath(const sp<IBinder::DeathRecipient> & recipient,
		void * cookie = nullptr, uint32_t flags = 0);
	sp<SurfaceControl> createSurface(const String8 & name, uint32_t w, uint32_t h,
		PixelFormat format, uint32_t flags = 0);

	static sp<IBinder> getBuiltInDisplay(int32_t id);
	static status_t getDisplayInfo(const sp<IBinder> & display, DisplayInfo * info);
	static void openGlobalTransaction();
	static void closeGlobalTransaction(bool synchronous = false);

private:
	mutable Mutex mLock;
	status_t mStatus;
	sp<RefBase> mClient;
	int & mComposer;
};

}

#endif
