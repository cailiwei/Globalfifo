package com.phicomm.globalfifo;

import com.phicomm.globalfifo.dao.UserWay;
import com.phicomm.globalfifo.dao.Users;
import com.phicomm.globalfifo.db.DatabaseHelper;

import android.app.Activity;
import android.content.Intent;
import android.database.sqlite.SQLiteDatabase;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

/**
 * 登录系统
 * @author Administrator
 *
 */
public class LogonActivity extends Activity {
    /** Called when the activity is first created. */
    private Button regButton = null;
    private Button loginButton = null;
    private EditText nameText = null;
    private EditText password = null;
    private String userName,userCode;
	@Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);		
        setContentView(R.layout.main);
        
        regButton = (Button)findViewById(R.id.reg);
        loginButton = (Button)findViewById(R.id.logon);
        nameText = (EditText)findViewById(R.id.username);
        password = (EditText)findViewById(R.id.usercode); 
    	//注册按钮
        regButton.setOnClickListener(new View.OnClickListener() {
			
			@Override
			public void onClick(View v) {
				Intent intent = new Intent();
				intent.setClass(LogonActivity.this,RegistActivity.class);
				LogonActivity.this.startActivity(intent);
			}
		});
        
        loginButton.setOnClickListener(new loginListener());   
    }
	
	//登录按钮
	protected class loginListener implements OnClickListener {

		Users user = new Users();
		boolean checked = false;
		@Override
		public void onClick(View v) {
			//先封装
			userName = nameText.getText().toString();
			userCode = password.getText().toString();
			user.setName(userName);
			user.setPass(userCode);
			// 再调用函数进行查询
			// 创建一个DatabaseHelper对象
			DatabaseHelper dbHelper = new DatabaseHelper(LogonActivity.this,"clw_users");
			// 只有调用了DatabaseHelper对象的getReadableDatabase()方法，
			// getWritableDatabase()方法之后，才会创建，或打开一个数据库
			SQLiteDatabase db = dbHelper.getReadableDatabase();
			checked = UserWay.queryDatabase(user, db);
			if (checked) {
				//System.out.println("恭喜你，登陆成功！");
				LogonActivity.this.finish();
				Toast.makeText(LogonActivity.this, "恭喜你，登陆成功！", Toast.LENGTH_SHORT).show();
				Intent intent = new Intent();
				intent.putExtra("name", user.getName());
				intent.putExtra("password", user.getPass());
				intent.setClass(LogonActivity.this, MainActivity.class);
				LogonActivity.this.startActivity(intent);
			} else {
				//System.out.println("对不起，密码错误，请重新登录！");	
				Toast.makeText(LogonActivity.this, "对不起，密码错误，请重新登录！", Toast.LENGTH_SHORT).show();
			}
		}		
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// TODO Auto-generated method stub
		menu.add(0, 1, 1, R.string.exit);
		menu.add(0, 2, 2, R.string.about);
		return super.onCreateOptionsMenu(menu);
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// TODO Auto-generated method stub
		if(item.getItemId()==1){
			finish();
		}
		return super.onOptionsItemSelected(item);
	}
}
