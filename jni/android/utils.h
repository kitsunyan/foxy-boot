#ifndef __ANDROID_UTILS__
#define __ANDROID_UTILS__

#include <errno.h>
#include <pthread.h>
#include <stdint.h>

namespace android {

typedef int32_t status_t;

enum {
	NO_ERROR = 0,
	PERMISSION_DENIED = -EPERM,
	NO_INIT = -ENODEV
};

class RefBase {
public:
	void incStrong(const void * id) const;
	void decStrong(const void * id) const;

	class weakref_type {};

protected:
	RefBase();
	virtual ~RefBase();

	virtual void onFirstRef();
	virtual void onLastStrongRef(const void * id);
	virtual bool onIncStrongAttempted(uint32_t flags, const void * id);
	virtual void onLastWeakRef(const void * id);

private:
	class weakref_impl;
	weakref_impl * const mRefs;
};

class String8 {
public:
	explicit String8(const char * o);
	size_t length() const;
private:
	const char * mString;
};

template <typename T> class wp {
private:
	T * m_ptr;
	RefBase::weakref_type * m_refs;
};

template<typename T> class sp {
public:
	inline sp(): m_ptr(nullptr) {}
	
	sp(T * other): m_ptr(other) {
		if (other) {
			other->incStrong(this);
		}
	}

	sp(const sp<T> & other): m_ptr(other.m_ptr) {
		if (m_ptr) {
			m_ptr->incStrong(this);
		}
	}

	sp(sp<T> && other): m_ptr(other.m_ptr) {
		other.m_ptr = nullptr;
	}

	~sp() {
		if (m_ptr) {
			m_ptr->decStrong(this);
		}
	}

	sp & operator=(T * other) {
		if (other) {
			other->incStrong(this);
		}
		if (m_ptr) {
			m_ptr->decStrong(this);
		}
		m_ptr = other;
		return *this;
	}

	sp & operator=(const sp<T> & other) {
		T * otherPtr(other.m_ptr);
		if (otherPtr) {
			otherPtr->incStrong(this);
		}
		if (m_ptr) {
			m_ptr->decStrong(this);
		}
		m_ptr = otherPtr;
		return *this;
	}

	sp & operator=(sp<T> && other) {
		if (m_ptr) {
			m_ptr->decStrong(this);
		}
		m_ptr = other.m_ptr;
		other.m_ptr = nullptr;
		return *this;
	}

	void clear() {
		if (m_ptr) {
			m_ptr->decStrong(this);
			m_ptr = nullptr;
		}
	}

	inline T & operator*() const {
		return *m_ptr;
	}

	inline T * operator->() const {
		return m_ptr;
	}

	inline T * get() const {
		return m_ptr;
	}

private:
	T * m_ptr;
};

enum {
	PRIORITY_DISPLAY = -4,
	PRIORITY_DEFAULT = 0
};

class Mutex {
public:
	Mutex();
	~Mutex();
private:
	pthread_mutex_t mMutex;
};

inline Mutex::Mutex() {
	pthread_mutex_init(&mMutex, nullptr);
}

inline Mutex::~Mutex() {
	pthread_mutex_destroy(&mMutex);
}

class Condition {
public:
	Condition();
	~Condition();
private:
	pthread_cond_t mCond;
};

inline Condition::Condition() {
	pthread_cond_init(&mCond, nullptr);
}

inline Condition::~Condition() {
	pthread_cond_destroy(&mCond);
}

typedef void * thread_id_t;

class Thread: virtual public RefBase {
public:
	Thread(bool canCallJava = true);
	virtual ~Thread();

	virtual status_t run(const char * name, int32_t priority = PRIORITY_DEFAULT, size_t stack = 0);
	virtual void requestExit();
	virtual status_t readyToRun();
	status_t requestExitAndWait();
	status_t join();
	bool isRunning() const;

protected:
	bool exitPending() const;

private:
	virtual bool threadLoop() = 0;

private:
	const bool mCanCallJava;
	thread_id_t mThread;
	mutable Mutex mLock;
	Condition mThreadExitedCondition;
	status_t mStatus;
	volatile bool mExitPending;
	volatile bool mRunning;
	sp<Thread> mHoldSelf;
	pid_t mTid;
};

}

#endif
