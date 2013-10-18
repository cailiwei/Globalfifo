package com.phicomm.globalfifo.dao;

import com.phicomm.globalfifo.db.DatabaseHelper;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;

//负责对数据库中的数据进行增删改查等操作的函数
public class UserWay {

	/**
	 * 创建数据库
	 * @param context
	 * @return
	 */
	public  static SQLiteDatabase createDatabase(Context context){
		//创建一个DatabaseHelper对象
		DatabaseHelper dbHelper = new DatabaseHelper(context,"clw_users");
		//只有调用了DatabaseHelper对象的getReadableDatabase()方法，或者是getWritableDatabase()方法之后，才会创建，或打开一个数据库
		SQLiteDatabase db = dbHelper.getWritableDatabase();
		return db;
	}
	
	/**
	 * 向指定的数据库中添加元素
	 * @param user
	 * @param db
	 * @return
	 */
	public  static boolean addDatabase(Users user, SQLiteDatabase db){
		//生成ContentValues对象
		ContentValues values = new ContentValues();
		//向该对象当中插入键值对，其中键是列名，值是希望插入到这一列的值，值必须和数据库当中的数据类型一致
		values.put("id", 1);
		values.put("name",user.getName());
		values.put("password", user.getPass());
		//调用insert方法，就可以将数据插入到数据库当中
		db.insert("user", null, values);
		System.out.println("insert a Database");
		return true;
	}
	
	/**
	 * 在指定数据库中查询指定元素是否存在或正确
	 * @param user
	 * @param db
	 * @return
	 */
	public static boolean queryDatabase(Users user,SQLiteDatabase db){
	
		boolean temp = false;
		Cursor cursor = db.query("user", new String[]{"name","password"}, "name=?", new String[]{user.getName()}, null, null, null);
		while (cursor.moveToNext()) {
			String password = cursor.getString(cursor.getColumnIndex("password"));
			System.out.println("query--->password" + password);
			if (user.getPass() == null) {
				System.out.println("密码为空，请输入密码！");
			} else {
				if(user.getPass().equals(password)){
					temp = true;
				}
			}
		}
		return temp;
	}
	
    //更新操作就相当于执行SQL语句当中的update语句
    //UPDATE table_name SET XXCOL=XXX WHERE XXCOL=XX...
	public static boolean updataDatabase(Context context,Users user, SQLiteDatabase db){

		ContentValues values = new ContentValues();
		values.put("name", "zhangsanfeng");
		//第一个参数是要更新的表名
		//第二个参数是一个ContentValeus对象
		//第三个参数是where子句
		db.update("user", values, "id=?", new String[]{"1"});
		return true;
	}
}
