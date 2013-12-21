package com.phicomm.globalfifo;

import android.util.Log;

import com.phicomm.globalfifo.aidl.IGlobalfifoService;

public class GlobalfifoService extends IGlobalfifoService.Stub {
	
	private static final String LOG_TAG = "Globalfifo";

	public static native boolean initDevNative();
	public static native int setDevParNative(char[] val);
	public static native int getDevParNative(char[] val, int count);

	static {
		try{
			System.loadLibrary("globalfifo_jni");
		}catch(UnsatisfiedLinkError e){
			Log.e(LOG_TAG, "Could not load library Globalfifo_JNI!");
		}
	}
	
	public GlobalfifoService() {
//		Log.i(LOG_TAG, "Globalfifo_Service constructor()");
	}

	public boolean init() {
		return initDevNative();
	}
	public int setVal(char[] val) {
		return setDevParNative(val);
	}	
	public int getVal(char[] val, int count) {
		return getDevParNative(val, count);
	}
};
