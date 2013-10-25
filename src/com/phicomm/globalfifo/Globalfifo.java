package com.phicomm.globalfifo;

import com.phicomm.globalfifo.GlobalfifoService;

public class Globalfifo {
	
	public static GlobalfifoService globalfifo= new GlobalfifoService();

	public Globalfifo() {
	}

	public static boolean init() {
		boolean err = globalfifo.init();
		return err;
	}

	public static int setVal(char[] val) {
		return globalfifo.setVal(val);
	}

	public static int getVal(char[] val, int count) {
		return globalfifo.getVal(val, count);
	}
}
