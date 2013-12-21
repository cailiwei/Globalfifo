#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#define  LOG_TAG "Globalfifo_jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#define TOTAL_SIZE 		0x1000 		/* TODO change the size by self */

#define DEVICE_NAME "/dev/param_fifo"
#define MODULE_NAME "globalfifo_jni"

#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

//struct globalfifo_module_t {
//	struct hw_module_t common;
//};

struct globalfifo_device_t {
//	struct hw_device_t common;
	int fd;
	int (*set_val)(struct globalfifo_device_t* dev, unsigned char* val, int count);
	int (*get_val)(struct globalfifo_device_t* dev, unsigned char* val, int count);
};

// struct globalfifo_device_t* globalfifo_device_init();
