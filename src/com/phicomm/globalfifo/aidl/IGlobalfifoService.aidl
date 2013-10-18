/* //device/java/android/android/os/IGlobalfifoService.aidl
 * Copyright (c) 2012, The Linux Foundation. All rights reserved.
 */

package android.os;

/** @hide */
interface IGlobalfifoService {
	void setVal(in char[] data);
	void getVal(in char[] data, int count);
}
