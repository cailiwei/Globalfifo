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

	public void setVal(char[] val) {
		globalfifo.setVal(val);
	}

	public void getVal(char[] val, int count) {
		globalfifo.getVal(val, count);
	}
}
