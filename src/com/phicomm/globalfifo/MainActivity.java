package com.phicomm.globalfifo;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;

import java.util.ArrayList;

public class MainActivity extends Activity {
	
	private final static String LOG_TAG = "globalfifo";

	private TextView start = null;
	private EditText content = null;
	private Button readButton = null;
	private Button writeButton = null;
	private Button clearButton = null;	
	private static boolean status = false;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.start);

		start = (TextView)findViewById(R.id.st);
		content = (EditText)findViewById(R.id.content);
		readButton = (Button)findViewById(R.id.read);
		writeButton = (Button)findViewById(R.id.write);
		clearButton = (Button)findViewById(R.id.clear);

		Intent intent = getIntent();
        String name = intent.getStringExtra("name");
        if (Globalfifo.init()) {
		    start.setText(name+", 恭喜您，登陆成功并且服务已启动，你可以进行数据操作了!!!"); 	           		
		    readButton.setOnClickListener(new ReadGlobalfifoListener());
		    writeButton.setOnClickListener(new writeGlobalfifoListener());
		    clearButton.setOnClickListener(new clearGlobalfifoListener());
			content.setVisibility(View.INVISIBLE);
		
            Log.i(LOG_TAG, "Globalfifo service Created.");
        } else {	
            start.setText(name+"，很抱歉，服务启动失败，无法进行读写等操作！"); 
			Log.e(LOG_TAG, "Globalfifo Service was not started.");
        }
	}
    class ReadGlobalfifoListener implements OnClickListener{
		@Override
		public void onClick(View v) {
			char[] val = new char[4096];
			int len = Globalfifo.getVal(val, 4096);
			ArrayList<Integer> b = new ArrayList<Integer>((int)val[0]); 
			for (int i = 0; i < val[0]; i++) {
				b.add((int)val[i+1]);
			}
			String result = "Read the "+ len +" data:\n";
			int index = val[0]+2;
			for (int i = 0; i < val[0]; i++) {
				result += "Array" + i + "[" + b.get(i) + "] = {\n\t";
				for(int temp = 0; temp < b.get(i); temp++ ) {
					String str = Integer.toHexString((int)val[index]);
					if (val[index++] < 16) str = "0" + str;
					result += "0x" + str + ", ";

					if ((temp+1) % 4 == 0) result += "\n\t";
				}
				result += "\n};\n";
			}
			content.setText(result);
			content.setCursorVisible(false);
			content.setVisibility(View.VISIBLE);
			content.setMovementMethod(ScrollingMovementMethod.getInstance());
			content.setSelection(content.getText().length(), content.getText().length());
			readButton.setEnabled(false);
		}   	
    }
    class writeGlobalfifoListener implements OnClickListener{
		@Override
		public void onClick(View v) {
			if (status) {
				char[] temp = {1,2,3};
				Globalfifo.setVal(temp);
				readButton.setEnabled(true);
			} else {
				start.setText("Now you can input data like this:\n 3, 1, 2, 3, 6...");
				content.setVisibility(View.VISIBLE);
				content.setFocusable(true);
				content.setCursorVisible(true);
				status = true;
			}
		}	
    }    
    class clearGlobalfifoListener implements OnClickListener{
		@Override
		public void onClick(View v) {
			content.setVisibility(View.INVISIBLE);
    		String text = "";
    		content.setText(text);

			status = false;
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
