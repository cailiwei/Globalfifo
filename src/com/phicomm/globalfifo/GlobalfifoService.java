/*
 *  Copyright (C) 2008 The Android Open Source Project
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

package com.phicomm.globalfifo;

import android.util.Log;

import com.phicomm.globalfifo.aidl.IGlobalfifoService;

public class GlobalfifoService extends IGlobalfifoService.Stub {
	
	private static final String LOG_TAG = "Globalfifo";

	static {
		try{
			System.loadLibrary("globalfifo_jni");
		}catch(UnsatisfiedLinkError e){
			System.err.println("Error:Could not load library Globalfifo_JNI!");
		}
	}

	public GlobalfifoService() {
		Log.i(LOG_TAG,"GlobalfifoService constructor()");
	}

	public boolean init() {
		return init_globalfifo_native();
	}

	public int setVal(char[] val) {
		return setVal_native(val);
	}	
	public int getVal(char[] val, int count) {
		return getVal_native(val, count);
	}

	private static native boolean init_globalfifo_native();
	private static native int setVal_native(char[] val);
	private static native int getVal_native(char[] val, int count);
};
