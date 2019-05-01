#ifndef __ANDROID_UTILS__
#define __ANDROID_UTILS__

#include <errno.h>
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

protected:
	RefBase();
	virtual ~RefBase();

private:
	void * const mRefs;
};

class String8 {
public:
	explicit String8(const char * o);

private:
	const char * mString;
};

template<typename T> class sp {
public:
	inline sp(): m_ptr(nullptr) {}
	sp(const sp<T> & other): m_ptr(other.m_ptr) {}

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

}

#endif
