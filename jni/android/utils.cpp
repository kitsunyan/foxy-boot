#include "utils.h"

using namespace android;

void RefBase::incStrong(const void * id) const {}
void RefBase::decStrong(const void * id) const {}
RefBase::RefBase(): mRefs(nullptr) {}
RefBase::~RefBase() {}

String8::String8(const char * o) {}
