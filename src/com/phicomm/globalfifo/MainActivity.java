package com.phicomm.globalfifo;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.RemoteException;
import android.widget.Button;
import android.widget.TextView;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;

public class MainActivity extends Activity {
	
	private final static String LOG_TAG = "globalfifo";

	private TextView start = null;
	private TextView content = null;
	private Button readButton = null;
	private Button writeButton = null;
	private Button clearButton = null;	

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.start);
		Intent intent = getIntent();

		start = (TextView)findViewById(R.id.st);
		content = (TextView)findViewById(R.id.content);
		readButton = (Button)findViewById(R.id.read);
		writeButton = (Button)findViewById(R.id.write);
		clearButton = (Button)findViewById(R.id.clear);

        String name = intent.getStringExtra("name");
        if (Globalfifo.init()) {
            start.setText(name+"，很抱歉，服务启动失败，无法进行读写等操作！"); 
			Log.e(LOG_TAG, "Globalfifo Service was not started.");
        } else {	
		    start.setText(name+", 恭喜您，登陆成功并且服务已启动，你可以进行数据操作了!!!"); 	           		
		    readButton.setOnClickListener(new ReadGlobalfifoListener());
		    writeButton.setOnClickListener(new writeGlobalfifoListener());
		    clearButton.setOnClickListener(new clearGlobalfifoListener());
		
            Log.i(LOG_TAG, "Globalfifo service Created.");
        }
	}
    class ReadGlobalfifoListener implements OnClickListener{
		@Override
		public void onClick(View v) {
			String text = "";
			char[] val = new char[50];
			Globalfifo.getVal(val, 50);
			for (int i = 0; i < 50; i++) {
				text = "0x" + val[i] + ",";
			}
			content.setText(text);
		}   	
    }
    class writeGlobalfifoListener implements OnClickListener{
		@Override
		public void onClick(View v) {
			char[] temp = {1,2,3};
			Globalfifo.setVal(temp);
		}  	
    }    
    class clearGlobalfifoListener implements OnClickListener{
		@Override
		public void onClick(View v) {
    		String text = "";
    		content.setText(text);
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
