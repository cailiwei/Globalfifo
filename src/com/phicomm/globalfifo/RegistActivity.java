package com.phicomm.globalfifo;

import android.app.Activity;
import android.database.sqlite.SQLiteDatabase;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import com.phicomm.globalfifo.dao.UserWay;
import com.phicomm.globalfifo.dao.Users;

public class RegistActivity extends Activity{

	private Button doneButton = null;
	private Button cancelButton = null;
	private EditText nameText = null;
	private EditText passText = null;
	private String name, pass;
	private SQLiteDatabase db = null;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);        
		db = UserWay.createDatabase(RegistActivity.this);
		setContentView(R.layout.regist);

		doneButton = (Button)findViewById(R.id.done);
		cancelButton = (Button)findViewById(R.id.cancel);
		nameText = (EditText)findViewById(R.id.name);
		passText = (EditText)findViewById(R.id.code);

		cancelButton.setOnClickListener(new OnClickListener() {		
			@Override
			public void onClick(View v) {
				finish();
			}
		});
		doneButton.setOnClickListener(new doneListener());
	}
	
	class doneListener implements OnClickListener{

		Users user = new Users();
		boolean checked = false;
		@Override
		public void onClick(View v) {
			name = nameText.getText().toString();
			pass = passText.getText().toString();
			user.setName(name);
			user.setPass(pass);
			checked = UserWay.addDatabase(user, db);
			if (checked){
				Toast.makeText(RegistActivity.this, R.string.login_success, Toast.LENGTH_SHORT).show();
			}
			finish();			
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
