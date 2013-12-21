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

public class LogonActivity extends Activity {
    /** Called when the activity is first created. */
	private static final String DATABASE_NAME = "globalfifoDatabase";
	private static final int VERSION = 2;
	
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

        regButton.setOnClickListener(new View.OnClickListener() {			
			@Override
			public void onClick(View v) {
				Intent intent = new Intent();
				intent.setClass(LogonActivity.this, RegistActivity.class);
				LogonActivity.this.startActivity(intent);
			}
		});
        
        loginButton.setOnClickListener(new loginListener());   
    }
	
	protected class loginListener implements OnClickListener {

		Users user = new Users();
		boolean checked = false;
		@Override
		public void onClick(View v) {
			userName = nameText.getText().toString();
			userCode = password.getText().toString();
			user.setName(userName);
			user.setPass(userCode);

			DatabaseHelper dbHelper = new DatabaseHelper(LogonActivity.this, DATABASE_NAME, VERSION);
			SQLiteDatabase db = dbHelper.getReadableDatabase();
			
			checked = UserWay.queryDatabase(user, db);	
			if (checked) {
				LogonActivity.this.finish();
				Toast.makeText(LogonActivity.this, R.string.logon_success, Toast.LENGTH_SHORT).show();
				Intent intent = new Intent();
				intent.putExtra("name", user.getName());
				intent.putExtra("password", user.getPass());
				intent.setClass(LogonActivity.this, MainActivity.class);
				LogonActivity.this.startActivity(intent);
			} else {
				Toast.makeText(LogonActivity.this, R.string.logon_failed, Toast.LENGTH_SHORT).show();
			}
		}		
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		menu.add(0, 1, 1, R.string.exit);
		menu.add(0, 2, 2, R.string.about);
		return super.onCreateOptionsMenu(menu);
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		if(item.getItemId()==1){
			finish();
		}
		return super.onOptionsItemSelected(item);
	}
}
