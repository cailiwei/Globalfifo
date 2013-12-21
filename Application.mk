# The ARMv7 is significanly faster due to the use of the hardware FPU
APP_PROJECT_PATH :=$(call my-dir)
APP_MODULES :=jni
APP_ABI := armeabi armeabi-v7a
APP_PLATFORM := android-19
