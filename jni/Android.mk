LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := cutils
LOCAL_SRC_FILES := android/cutils.c
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := gui
LOCAL_SRC_FILES := android/gui.cpp
LOCAL_SHARED_LIBRARIES := utils
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := utils
LOCAL_SRC_FILES := android/utils.cpp
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := font.c source-kmsg.c surface.cpp main.c
LOCAL_C_INCLUDES := jni/android
LOCAL_CFLAGS := -DGL_GLEXT_PROTOTYPES -Wall -Wextra -Wno-unused-parameter
LOCAL_SHARED_LIBRARIES := cutils gui utils
LOCAL_LDLIBS := -lEGL -lGLESv1_CM
LOCAL_MODULE := bootanimation
include $(BUILD_EXECUTABLE)
