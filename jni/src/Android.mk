LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := globalfifo_jni

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

# Add your application source files here...
LOCAL_SRC_FILES := com_phicomm_globalfifo_GlobalfifoService.cpp \
					  globalfifo_main.c

LOCAL_LDLIBS := -ldl -lGLESv1_CM -lGLESv2 -llog -landroid

include $(BUILD_SHARED_LIBRARY)
