LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libglobalfifo_jni

LOCAL_SRC_FILES := GlobalfifoProc.c \
				   globalfifo_init.c

LOCAL_LDLIBS    := -llog -ljnigraphics

include $(BUILD_SHARED_LIBRARY)
