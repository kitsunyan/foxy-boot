#include "binder.h"

using namespace android;

IPCThreadState * IPCThreadState::self() { return nullptr; }
void IPCThreadState::joinThreadPool(bool isMain) {}
void IPCThreadState::stopProcess(bool immediate) {}

sp<ProcessState> ProcessState::self() { return nullptr; }
void ProcessState::startThreadPool() {}
