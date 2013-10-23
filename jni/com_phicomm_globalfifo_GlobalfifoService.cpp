/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "GlobalfifoService"

#include "jni.h"
#include "JNIHelp.h"
// #include "android_runtime/AndroidRuntime.h"

#include "globalfifo.h"

#include <utils/misc.h>
#include <utils/Log.h>
#include <hardware/hardware.h>

#include <stdio.h>

//namespace android {
extern "C" struct globalfifo_device_t* globalfifo_device_init();

struct globalfifo_device_t* globalfifo_device = NULL;

static void globalfifo_setVal(JNIEnv* env, jobject clazz, jcharArray buffer) {
	jchar *array;
	jint len = env->GetArrayLength(buffer);

	if(!globalfifo_device) {
		ALOGI("Globalfifo JNI: device is not open.");
		return;
	}

	array = env->GetCharArrayElements(buffer, NULL);
	if (array == NULL) {
		ALOGE("jni_radio_send: GetCharArrayElements error.");
	}

	globalfifo_device->set_val(globalfifo_device, (unsigned char *)array, (int)len);

	free(array);
}

static void globalfifo_getVal(JNIEnv* env, jobject clazz, jcharArray buffer, jint count) {

	jchar *array;

	if(!globalfifo_device) {
		ALOGI("Globalfifo JNI: device is not open.");
	}
	array = (jchar *)calloc(count, sizeof(jboolean));
	if (array == NULL) {
		ALOGE("jni_radio_recive: calloc error.");
	}
	globalfifo_device->get_val(globalfifo_device, (unsigned char*)array, count);

	env->SetCharArrayRegion(buffer, 0, count, array);
	free(array);
}

static jboolean init_globalfifo_native(JNIEnv* env, jclass clazz) {

	jboolean ret = false;

	globalfifo_device = globalfifo_device_init();
	if (globalfifo_device != NULL) {
		ALOGE("Globalfifo JNI: globalfifo device open successed.");
		ret = true;
	}
	return ret;
}

static const JNINativeMethod method_table[] = {
	{"init_globalfifo_native", "()Z", (void*)init_globalfifo_native },
	{"setVal_native", "([C)V", (void*)globalfifo_setVal },
	{"getVal_native", "([CI)V", (void*)globalfifo_getVal },
};

int register_android_server_GlobalfifoService(JNIEnv *env)
{
	return jniRegisterNativeMethods(env, "com/phicomm/globalfifo/GlobalfifoService",
			method_table, NELEM(method_table));
}

/* This function will be call when the library first be load.
 * You can do some init in the libray. return which version jni it support.
 */
jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv* env = NULL;
	jint result = -1;

	if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
		ALOGE("ERROR: GetEnv failed\n");
		goto fail;
	}

	if (register_android_server_GlobalfifoService(env) < 0) {
		ALOGE("ERROR: FingerPrint native registration failed\n");
		goto fail;
	}

	/* success -- return valid version number */
	result = JNI_VERSION_1_4;
fail:
	return result;
}
//};
