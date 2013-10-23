package com.phicomm.globalfifo.aidl;

/** @hide */
interface IGlobalfifoService {
	void setVal(in char[] data);
	void getVal(in char[] data, in int count);
}
