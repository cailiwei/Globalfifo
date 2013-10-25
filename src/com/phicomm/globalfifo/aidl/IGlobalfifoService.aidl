package com.phicomm.globalfifo.aidl;

/** @hide */
interface IGlobalfifoService {
	boolean init();
	int setVal(in char[] data);
	int getVal(in char[] data, in int count);
}
