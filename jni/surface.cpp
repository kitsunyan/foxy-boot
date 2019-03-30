#include "surface.h"

#include <sys/resource.h>
#include <android/gui.h>
#include <unistd.h>

class SurfaceThread: public android::Thread, public android::IBinder::DeathRecipient {
public:
	SurfaceThread(surface_cb_t * surface_cb): android::Thread(false), surfaceCallbacks(surface_cb),
		client(new android::SurfaceComposerClient()) {
		surface_cb->priv = this;
	}

	inline bool exitPendingPublic() {
		return exitPending();
	}

private:
	surface_cb_t * surfaceCallbacks;

	android::sp<android::SurfaceComposerClient> client;
	android::sp<android::SurfaceControl> control;
	android::sp<android::Surface> surface;

	virtual void onFirstRef() {
		android::status_t err = client->linkToComposerDeath(this);
		if (err == android::NO_ERROR) {
			run("SurfaceThread", android::PRIORITY_DISPLAY);
		}
	}

	virtual void binderDied(const android::wp<android::IBinder> & who) {
		kill(getpid(), SIGKILL);
		requestExit();
	}

	virtual android::status_t readyToRun() {
		android::sp<android::IBinder> dtoken(android::SurfaceComposerClient::
			getBuiltInDisplay(android::ISurfaceComposer::eDisplayIdMain));
		android::DisplayInfo dinfo;
		android::status_t status = android::SurfaceComposerClient::getDisplayInfo(dtoken, &dinfo);
		if (status != android::NO_ERROR) {
			return android::PERMISSION_DENIED;
		}

		android::sp<android::SurfaceControl> control = client->createSurface(android::String8("SurfaceThread"),
			dinfo.w, dinfo.h, android::PIXEL_FORMAT_RGBA_8888);

		android::SurfaceComposerClient::openGlobalTransaction();
		control->setLayer(0x40000000);
		android::SurfaceComposerClient::closeGlobalTransaction();

		android::sp<android::Surface> surface = control->getSurface();
		if (!surfaceCallbacks->gl_prepare(surfaceCallbacks, surface.get(), dinfo.density)) {
			return android::NO_INIT;
		}

		SurfaceThread::control = control;
		SurfaceThread::surface = surface;
		return android::NO_ERROR;
	}

	virtual bool threadLoop() {
		surfaceCallbacks->gl_loop(surfaceCallbacks);
		surface.clear();
		control.clear();
		android::IPCThreadState::self()->stopProcess();
		return false;
	}
};

void surface_run(surface_cb_t * surface_cb) {
	setpriority(PRIO_PROCESS, 0, android::PRIORITY_DISPLAY);
	android::sp<android::ProcessState> proc(android::ProcessState::self());
	proc->startThreadPool();
	android::sp<SurfaceThread> thread(new SurfaceThread(surface_cb));
	android::IPCThreadState::self()->joinThreadPool();
	setpriority(PRIO_PROCESS, 0, android::PRIORITY_DEFAULT);
}

int surface_exit_pending(struct surface_cb_t * surface_cb) {
	SurfaceThread * surfaceThread = (SurfaceThread *) surface_cb->priv;
	return surfaceThread->exitPendingPublic();
}

void surface_exit(struct surface_cb_t * surface_cb) {
	SurfaceThread * surfaceThread = (SurfaceThread *) surface_cb->priv;
	surfaceThread->requestExit();
}
