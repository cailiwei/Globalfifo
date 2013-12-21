package com.phicomm.globalfifo.db;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteDatabase.CursorFactory;
import android.database.sqlite.SQLiteOpenHelper;

public class DatabaseHelper extends SQLiteOpenHelper {
	
	private static final int VERSION = 1;

	public DatabaseHelper(Context context, String name, CursorFactory factory, 
			int version) {
		super(context, name, factory, version);
	}
	public DatabaseHelper(Context context, String name){
		this(context, name, VERSION);
	}
	public DatabaseHelper(Context context, String name, int version){
		this(context, name, null, version);
	}

	@Override
	public void onCreate(SQLiteDatabase db) {
		db.execSQL("create table user(id int, name varchar(20), password varchar(16))");
//		System.out.println("Create a Database!");
	}

	@Override
	public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {		
//		System.out.println("update a Database");
	}

}
