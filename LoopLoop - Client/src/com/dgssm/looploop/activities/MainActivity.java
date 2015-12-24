package com.dgssm.looploop.activities;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.widget.Button;

import com.dgssm.looploop.R;

public class MainActivity extends Activity {

	private static final String TAG = "MainActivity";

	private Button btnOneManBand, btnRealBand;
		
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		setContentView(R.layout.main_activity);
		
		btnOneManBand = (Button) findViewById(R.id.btnOneManBand);
		btnRealBand = (Button) findViewById(R.id.btnRealBand);
		btnOneManBand.setOnClickListener(mListener);
		btnRealBand.setOnClickListener(mListener);
		
	}
	
	private View.OnClickListener mListener = new View.OnClickListener(){
		@Override
		public void onClick(View v) {
			if(v == btnOneManBand){
				Log.d(TAG, "btnOneManBand");
				Intent intent = new Intent(MainActivity.this, SettingActivity.class);
				startActivity(intent);
				overridePendingTransition(R.anim.fade, R.anim.hold);
			}
			else if(v == btnRealBand){
				Log.d(TAG, "btnRealBand");
				Intent intent = new Intent(MainActivity.this, LoopLoopMultiActivity.class);
				startActivity(intent);
				overridePendingTransition(R.anim.fade, R.anim.hold);
			}
		}
	};

}