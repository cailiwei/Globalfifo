package com.phicomm.globalfifo.dao;

import com.phicomm.globalfifo.db.DatabaseHelper;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.util.Log;

public class UserWay {
	
	private static final String LOG_TAG = "Globalfifo";
	private static final String DATABASE_NAME = "globalfifoDatabase";
	
	public static SQLiteDatabase createDatabase(Context context){
		
		DatabaseHelper dbHelper = new DatabaseHelper(context, DATABASE_NAME);
		
		SQLiteDatabase db = dbHelper.getWritableDatabase();
		
		return db;
	}
	
	public static boolean addDatabase(Users user, SQLiteDatabase db){

		ContentValues values = new ContentValues();
		values.put("id", 1);
		values.put("name",user.getName());
		values.put("password", user.getPass());

		db.insert("user", null, values);
		return true;
	}
	
	public static boolean queryDatabase(Users user, SQLiteDatabase db){
	
		boolean temp = false;		
		Cursor cursor = db.query("user", new String[]{"name","password"}, "name=?", new String[]{user.getName()}, null, null, null);
		while (cursor.moveToNext()) {
			String password = cursor.getString(cursor.getColumnIndex("password"));
			Log.i(LOG_TAG, user.getName() + "--->password: " + password);
			if (user.getPass() == null) {
				Log.e(LOG_TAG, "密码为空，请输入密码！");
			} else {
				if(user.getPass().equals(password)){
					temp = true;
				}
			}
		}
		return temp;
	}
	
	/* TODO not used */
	public static boolean updataDatabase(Context context, Users user, SQLiteDatabase db){
	    //UPDATE table_name SET XXCOL=XXX WHERE XXCOL=XX...
		ContentValues values = new ContentValues();
		values.put("name", "zhangsanfeng");
		
		//第一个参数是要更新的表名
		//第二个参数是一个ContentValeus对象
		//第三个参数是where子句
		db.update("user", values, "id=?", new String[]{"1"});
		return true;
	}
}
