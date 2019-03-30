#ifndef __ANDROID_BINDER__
#define __ANDROID_BINDER__

#include <android/utils.h>

namespace android {

class IBinder: public virtual RefBase {
public:
	class DeathRecipient: public virtual RefBase {
	public:
		virtual void binderDied(const wp<IBinder> & who) = 0;
	};
};

class IPCThreadState {
public:
	static IPCThreadState * self();
	void joinThreadPool(bool isMain = true);
	void stopProcess(bool immediate = true);
};

class ProcessState: public virtual RefBase {
public:
	static sp<ProcessState> self();
	void startThreadPool();
};

}

#endif
