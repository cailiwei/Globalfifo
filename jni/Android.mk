LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
# LOCAL_PRELINK_MODULE := false
LOCAL_LDLIBS += -lpthread

# LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_SHARED_LIBRARIES := liblog libcutils libnativehelper

LOCAL_SRC_FILES := com_phicomm_globalfifo_GlobalfifoService.cpp \
				   globalfifo_main.c

LOCAL_MODULE := libglobalfifo_jni

LOCAL_C_INCLUDES += \
				$(TARGET_OUT_HEADERS)/common/inc \
				$(JNI_H_INCLUDE) \

include $(BUILD_SHARED_LIBRARY)
