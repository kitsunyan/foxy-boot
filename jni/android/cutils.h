#ifndef __ANDROID_CUTILS__
#define __ANDROID_CUTILS__

#include <sys/system_properties.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PROPERTY_VALUE_MAX PROP_VALUE_MAX

int property_get(const char * key, char * value, const char * default_value);

#ifdef __cplusplus
}
#endif

#endif
