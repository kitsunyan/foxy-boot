#include "utils.h"

using namespace android;

void RefBase::incStrong(const void * id) const {}
void RefBase::decStrong(const void * id) const {}
RefBase::RefBase(): mRefs(nullptr) {}
RefBase::~RefBase() {}
void RefBase::onFirstRef() {}
void RefBase::onLastStrongRef(const void * id) {}
bool RefBase::onIncStrongAttempted(uint32_t flags, const void * id) { return false; }
void RefBase::onLastWeakRef(const void * id) {}

String8::String8(const char * o) {}
size_t String8::length() const { return 0; }

Thread::Thread(bool canCallJava): mCanCallJava(canCallJava) {}
Thread::~Thread() {}
status_t Thread::run(const char * name, int32_t priority, size_t stack) { return 0; }
void Thread::requestExit() {}
status_t Thread::readyToRun() { return 0; }
status_t Thread::requestExitAndWait() { return 0; }
status_t Thread::join() { return 0; }
bool Thread::isRunning() const { return false; }
bool Thread::exitPending() const { return false; }
