package com.dgssm.looploop.activities;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.widget.ImageView;

import com.dgssm.looploop.R;
import com.dgssm.looploop.utils.Constants;

public class SettingActivity extends Activity {

	private static final String TAG = "SettingActivity";

	private ImageView btnRecTime[] = new ImageView[6];
	
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		setContentView(R.layout.setting_activity);
		
		btnRecTime[0] = (ImageView) findViewById(R.id.btnRecTime1);
		btnRecTime[1] = (ImageView) findViewById(R.id.btnRecTime2);
		btnRecTime[2] = (ImageView) findViewById(R.id.btnRecTime3);
		btnRecTime[3] = (ImageView) findViewById(R.id.btnRecTime4);
		btnRecTime[4] = (ImageView) findViewById(R.id.btnRecTime5);
		btnRecTime[5] = (ImageView) findViewById(R.id.btnRecTime6);
		for(int i=0; i<6; i++){
			btnRecTime[i].setOnClickListener(mListener);
		}
	}
	
	private View.OnClickListener mListener = new View.OnClickListener(){
		@Override
		public void onClick(View v) {
			if(v == btnRecTime[0])		  {			Constants.recTime = 5000;			}
			else if(v == btnRecTime[1]){			Constants.recTime = 6000;			}
			else if(v == btnRecTime[2]){			Constants.recTime = 7000;			}
			else if(v == btnRecTime[3]){			Constants.recTime = 8000;			}
			else if(v == btnRecTime[4]){			Constants.recTime = 9000;			}
			else if(v == btnRecTime[5]){			Constants.recTime = 10000;			}
			
			Log.d(TAG, "recTime = " + Constants.recTime);
			Intent intent = new Intent(SettingActivity.this, LoopLoopActivity.class);
			startActivity(intent);
			overridePendingTransition(R.anim.fade, R.anim.hold);
		}
	};

}