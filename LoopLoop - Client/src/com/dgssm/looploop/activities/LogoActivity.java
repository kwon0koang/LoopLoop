package com.dgssm.looploop.activities;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.Window;

import com.dgssm.looploop.R;

public class LogoActivity extends Activity {
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		setContentView(R.layout.logo_activity);
		
		Handler handler = new Handler(){
			public void handleMessage(Message msg){
				super.handleMessage(msg);;
				Intent intent = new Intent(LogoActivity.this, MainActivity.class);
				startActivity(intent);
				overridePendingTransition(R.anim.fade, R.anim.hold);
				finish();
			}
		};
		
		handler.sendEmptyMessageDelayed(0, 1000);
	}
}
