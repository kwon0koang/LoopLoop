package com.dgssm.looploop.activities;

import java.io.File;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
//import android.util.Log;
import android.view.View;
import android.view.Window;
import android.widget.ImageView;
import android.widget.SeekBar;

import com.dgssm.looploop.R;
import com.dgssm.looploop.nativeaudio.NativeAudio;
import com.dgssm.looploop.utils.Constants;
import com.dgssm.looploop.waveform.EncodePCMtoM4A;
import com.dgssm.looploop.waveform.SoundFile;
import com.dgssm.looploop.waveform.WaveformView;

public class LoopLoopActivity extends Activity {
	
    private static final String TAG = "LoopLoopActivity";
        
    static final int CLIP_NONE = 0;;
    static final int CLIP_PLAYBACK = 4;
    
    private ImageView btnRec[] = new ImageView[5];
    private int recState[] = new int[5];
    private WaveformView waveformView[] = new WaveformView[5];
    private SeekBar sbVolume[] = new SeekBar[5];
    private ImageView btnMute[] = new ImageView[5];
    private ImageView btnReverb[] = new ImageView[5];
    private boolean muteState[] = new boolean[5];
    private boolean reverbState[] = new boolean[5];
    
    // for waveform
	private boolean recFlag = false;
	private boolean recWaitFlag = false;
	private SoundFile mSoundFile;
	private float mDensity;
	private int trackNum = 0;
	private String trackPath;
	private boolean chkFinishRecord = false;
	private File mFile;
    private String mFilename;	
        
    private MyThread mMyThread = null;
	
    //=============================================================================================================    
	
    /** Called when the activity is first created. */
	@Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.activity_looploop_main);

        // initialize native audio system
        NativeAudio.createEngine();
        NativeAudio.createBufferQueueAudioPlayer();
        NativeAudio.decideRecordTime(Constants.recTime);
        NativeAudio.createAudioRecorder();

        btnRec[0] = (ImageView) findViewById(R.id.rec1Button);
        btnRec[1] = (ImageView) findViewById(R.id.rec2Button);
        btnRec[2] = (ImageView) findViewById(R.id.rec3Button);
        btnRec[3] = (ImageView) findViewById(R.id.rec4Button);
        btnRec[4] = (ImageView) findViewById(R.id.rec5Button);
        for(int i=0; i<Constants.ALL_TRACK; i++){
        	btnRec[i].setOnClickListener(mListener);
        }
        
        waveformView[0] = (WaveformView) findViewById(R.id.waveformView1);
        waveformView[1] = (WaveformView) findViewById(R.id.waveformView2);
        waveformView[2] = (WaveformView) findViewById(R.id.waveformView3);
        waveformView[3] = (WaveformView) findViewById(R.id.waveformView4);
        waveformView[4] = (WaveformView) findViewById(R.id.waveformView5);
        
        for (int i = 0; i < Constants.ALL_TRACK; i++){
        	recState[i] = Constants.STATE_REC;
        }
        
        sbVolume[0] = (SeekBar) findViewById(R.id.sbVolume1);
        sbVolume[1] = (SeekBar) findViewById(R.id.sbVolume2);
        sbVolume[2] = (SeekBar) findViewById(R.id.sbVolume3);
        sbVolume[3] = (SeekBar) findViewById(R.id.sbVolume4);
        sbVolume[4] = (SeekBar) findViewById(R.id.sbVolume5);
        for(int i=0; i<Constants.ALL_TRACK; i++){
        	sbVolume[i].setOnSeekBarChangeListener(new mSeekbarChangeListener(i));
        }

        btnMute[0] = (ImageView) findViewById(R.id.mute1Button);
        btnMute[1] = (ImageView) findViewById(R.id.mute2Button);
        btnMute[2] = (ImageView) findViewById(R.id.mute3Button);
        btnMute[3] = (ImageView) findViewById(R.id.mute4Button);
        btnMute[4] = (ImageView) findViewById(R.id.mute5Button);
        for(int i=0; i<Constants.ALL_TRACK; i++){
        	btnMute[i].setOnClickListener(muteListener);
        }
        
        btnReverb[0] = (ImageView) findViewById(R.id.rev1Button);
        btnReverb[1] = (ImageView) findViewById(R.id.rev2Button);
        btnReverb[2] = (ImageView) findViewById(R.id.rev3Button);
        btnReverb[3] = (ImageView) findViewById(R.id.rev4Button);
        btnReverb[4] = (ImageView) findViewById(R.id.rev5Button);
        for(int i=0; i<Constants.ALL_TRACK; i++){
        	btnReverb[i].setOnClickListener(reverbListener);
        }
        
        for(int i=0; i<Constants.ALL_TRACK; i++){
        	reverbState[i] = false;
        	muteState[i] = false;
        }
        
        if (mMyThread == null) {
        	mMyThread = new MyThread();
        	mMyThread.start();
        }

    }

    /** Called when the activity is about to be destroyed. */
    @Override
    protected void onPause()
    {
        // turn off all audio
    	NativeAudio.selectClip(CLIP_NONE, 0, 999);
    	super.onPause();
    }

    /** Called when the activity is about to be destroyed. */
    @Override
    protected void onDestroy()
    {
    	if (mMyThread != null) {
			mMyThread.stop();
			mMyThread = null;
    	}
    	
    	NativeAudio.shutdown();
    	
        super.onDestroy();
    }
    
    
    
    
    
    
    
    
    
    
    //=============================================================================
    // ClickListener ===================================================================
    //=============================================================================
    private View.OnClickListener mListener = new View.OnClickListener() {
		@Override
		public void onClick(View v) {
			
			
			
			// 레코딩 중인데 레코드 버튼 눌렀을 때
			if(recFlag == true){
				Log.d(TAG, "Already recording");
			}
			// 처음 레코드 마치고, 플레이 중인데 레코드 버튼 눌렀을 때
			else{
				if(recWaitFlag == true){
					Log.d(TAG, "Already record waiting");
					// 만약, 플레이 도중에 다시 한번 누르면 없었던 일로 함
					if(recState[trackNum] == Constants.STATE_REC_WAIT){
						recState[trackNum] = Constants.STATE_REC;
						redrawBtnRec();
						recWaitFlag = false;
						for (int i = 0; i < Constants.ALL_TRACK; i++){
							if (i == trackNum) {
								continue;
							}
							
							btnRec[i].setClickable(true);
						}
					}
					return;
				}
				recWaitFlag = true;
				
				if (v == btnRec[0])		 { trackNum = 0; trackPath = Constants.M4A_PATHS[0]; }
				else if (v == btnRec[1]) { trackNum = 1; trackPath = Constants.M4A_PATHS[1]; }
				else if (v == btnRec[2]) { trackNum = 2; trackPath = Constants.M4A_PATHS[2]; }
				else if (v == btnRec[3]) { trackNum = 3; trackPath = Constants.M4A_PATHS[3]; }
				else if (v == btnRec[4]) { trackNum = 4; trackPath = Constants.M4A_PATHS[4]; }
				
				// 선택된 트랙 대기 상태로 바꿈
				recState[trackNum] = Constants.STATE_REC_WAIT;
				redrawBtnRec();
				
				// 플레이 끝나면 레코딩 상태로 바꾸고, 레코딩 시작
				//NativeAudio.startRecording(trackNum);
			}
			// 처음 레코드했을 때
			/*else{
				recordedFirstFlag = true;
				recFlag = true;
				
				if (v == btnRec[0])		 { trackNum = 0; trackPath = Constants.M4A_PATHS[0]; }
				else if (v == btnRec[1]) { trackNum = 1; trackPath = Constants.M4A_PATHS[1]; }
				else if (v == btnRec[2]) { trackNum = 2; trackPath = Constants.M4A_PATHS[2]; }
				else if (v == btnRec[3]) { trackNum = 3; trackPath = Constants.M4A_PATHS[3]; }
				else if (v == btnRec[4]) { trackNum = 4; trackPath = Constants.M4A_PATHS[4]; }
				
				recState[trackNum] = Constants.STATE_REC_ING;
				redrawBtnRec();
				
				
	            NativeAudio.startRecording(trackNum);
	        }*/
			
			for (int i = 0; i < Constants.ALL_TRACK; i++){
				if (i == trackNum) {
					continue;
				}
				
				btnRec[i].setClickable(false);
			}
		}
	};		
	
	private View.OnClickListener reverbListener = new View.OnClickListener() {
		@Override
		public void onClick(View v) {
			int selected = 0;
			if(v == btnReverb[0]){			selected = 0;		}
			else if(v == btnReverb[1]){	selected = 1;		}
			else if(v == btnReverb[2]){	selected = 2;		}			
			else if(v == btnReverb[3]){	selected = 3;		}
			else if(v == btnReverb[4]){	selected = 4;		}
			
			reverbState[selected] = !reverbState[selected]; 
			NativeAudio.enableReverb(reverbState[selected], selected);
			if(reverbState[selected])
				btnReverb[selected].setImageResource(R.drawable.reverb_o);
			else
				btnReverb[selected].setImageResource(R.drawable.reverb_x);
			
			Log.d(TAG, "reverbState " + selected + " = " + reverbState[selected]);
		}
	};
	
	private View.OnClickListener muteListener = new View.OnClickListener() {
		@Override
		public void onClick(View v) {
			int selected = 0;
			if(v == btnMute[0]){			selected = 0;		}
			else if(v == btnMute[1]){		selected = 1;		}
			else if(v == btnMute[2]){		selected = 2;		}			
			else if(v == btnMute[3]){		selected = 3;		}
			else if(v == btnMute[4]){		selected = 4;		}
			
			muteState[selected] = !muteState[selected]; 
			NativeAudio.setMuteUriAudioPlayer(muteState[selected], selected);
			if(muteState[selected])
				btnMute[selected].setImageResource(R.drawable.mute_o);
			else
				btnMute[selected].setImageResource(R.drawable.mute_x);
			
			Log.d(TAG, "btnMute " + selected + " = " + btnMute[selected]);
		}
	};
	//=============================================================================
    // ClickListener end ===================================================================
    //=============================================================================
		
	private class mSeekbarChangeListener implements SeekBar.OnSeekBarChangeListener{
		private int lastProgress = 100;
		private int trackNum = 0;
		private mSeekbarChangeListener(int trackNum){
			this.trackNum = trackNum;
		}
		
		@Override
		public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
	        lastProgress = progress;
	        int attenuation = 100 - lastProgress;
	        int millibel = attenuation * -50;
	        NativeAudio.setVolumeUriAudioPlayer(millibel, trackNum);
	    }
		@Override
	    public void onStopTrackingTouch(SeekBar seekBar)  {}
		@Override
		public void onStartTrackingTouch(SeekBar seekBar) {}
	}
	
	
	
	
	
	
	
	
	
	@SuppressLint("HandlerLeak")
	private Handler mHandler = new Handler(){
		@Override
		public void handleMessage(Message msg) {
			super.handleMessage(msg);
			if(msg.what == Constants.CHK_FINISH_RECORD){
				// new chkFinishEncodeThread().start();
				
				NativeAudio.setMuteUriAudioPlayer(false, trackNum);
				/*EncodePCMtoM4A.chkFinishEncode = false;
	            mHandler.post(new waveformDrawRunnable(trackPath));*/
	            recFlag = false;
	            recWaitFlag = false;
				chkFinishRecord = false;				
			}
			else if(msg.what == Constants.CHK_FINISH_ENCODE){
				NativeAudio.setMuteUriAudioPlayer(false, trackNum);
				EncodePCMtoM4A.chkFinishEncode = false;
	            mHandler.post(new waveformDrawRunnable(trackPath));
	            recFlag = false;
	            recWaitFlag = false;
				chkFinishRecord = false;
				
				recState[trackNum] = Constants.STATE_REC;
				redrawBtnRec();
				
				//NativeAudio.selectClip(CLIP_PLAYBACK, 1);
				
			}
			else if(msg.what == Constants.CHK_FINISH_PLAY){
				recState[trackNum] = Constants.STATE_REC_ING;
				redrawBtnRec();
				
				// 여기서 null 만들어주고 다시 레코딩
				NativeAudio.setMuteUriAudioPlayer(true, trackNum);
	            NativeAudio.startRecording(trackNum);
	            
	            new chkFinishRecordThread().start();
			}
			else if(msg.what == Constants.REDRAW_BTN_REC){
				redrawBtnRec();
			}
			else if(msg.what == Constants.DRAW_WAVEFORM){
				new chkFinishRecordThread().start();
			}
		}
	};
	private class chkFinishRecordThread extends Thread {
        @Override
        public void run() {
        	while(chkFinishRecord == false){		// 레코드 끝나길 기다려요. 뀨
                chkFinishRecord = NativeAudio.chkFinishRecord();
            }
        	//mHandler.sendEmptyMessage(Constants.CHK_FINISH_RECORD);
        	EncodePCMtoM4A.myEncode(trackNum);
        	/*try {
				Thread.sleep(500);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}*/
        	mHandler.post(new waveformDrawRunnable(trackPath));
        }
    }
	
	/*private class chkFinishEncodeThread extends Thread {
        @Override
        public void run() {
        	while(EncodePCMtoM4A.chkFinishEncode == false){		// 인코딩 끝나길 기다려요. 뀨
            	EncodePCMtoM4A.myEncode(trackNum);
            }
        	mHandler.sendEmptyMessage(Constants.CHK_FINISH_ENCODE);
        }
    }
	private class chkFinishPlayThread extends Thread {
        @Override
        public void run() {
        	chkFinishPlay = NativeAudio.chkFinishPlay();
        	boolean tmpFlag = !chkFinishPlay; 
        	
        	while(chkFinishPlay != tmpFlag){		// 플레이 끝나길 기다려요. 뀨
        		chkFinishPlay = NativeAudio.chkFinishPlay();
        		// Log.d(TAG, "chkFinishPlay = " + chkFinishPlay + " / tmpFlag = " + tmpFlag);
            }
        	mHandler.sendEmptyMessage(Constants.CHK_FINISH_PLAY);
        }
    }*/
	
	
	private class waveformDrawRunnable implements Runnable{
		waveformDrawRunnable(String filename){
			mFilename = filename;
		}
		@Override
		public void run() {
			try {
				//Log.d(TAG, "trackPath = " + mFilename);
	        	mFile = new File(mFilename);
	        	mSoundFile = SoundFile.create(mFile.getAbsolutePath(), null);
		        OpenSoundFile();
			} catch (Exception e) {
				e.printStackTrace();
			} 
		}
	}
	
	private void OpenSoundFile() {
		waveformView[trackNum].setSoundFile(mSoundFile);
		waveformView[trackNum].recomputeHeights(mDensity);
    }
	
	
	
	
	
	private void redrawBtnRec(){
		//Log.d(TAG, "redrawBtnRec()");
		for(int i=0; i<Constants.ALL_TRACK; i++){
			if(recState[i] == Constants.STATE_REC){
				btnRec[i].setImageResource(R.drawable.rec);
				//Log.d(TAG, "i = " + i + " / state = " + recState[i]);
			}
			else if(recState[i] == Constants.STATE_REC_WAIT){
				btnRec[i].setImageResource(R.drawable.rec_wait);
				//Log.d(TAG, "i = " + i + " / state = " + recState[i]);
			} 
			else if(recState[i] == Constants.STATE_REC_ING){
				btnRec[i].setImageResource(R.drawable.rec_ing);
				//Log.d(TAG, "i = " + i + " / state = " + recState[i]);
			}
		}
	}
	
	private void applyVolumeAndMuteAndReverb(){
		Log.d(TAG, "====================================");
		for(int i=0; i<Constants.ALL_TRACK; i++){
			// Volume set
			int attenuation = 100 - sbVolume[i].getProgress();
	        int millibel = attenuation * -50;
	        //NativeAudio.setVolumeUriAudioPlayer(millibel, trackNum);
	        NativeAudio.setVolumeUriAudioPlayer(millibel, i);
	        Log.d(TAG, "millibel = " + millibel + "  /  track = " + i);
	        
	        /*// Mute랑 Reverb 둘다 켜지니 소리나는 예외 상황 발생
 			if(muteState[i] && reverbState[i]){		// 옵션 두개 다 켜져 있을 때, 강제로 볼륨 0
 				NativeAudio.setVolumeUriAudioPlayer(-5000, trackNum);		// -5000 ~ 0
 				Log.d(TAG, i+" AAAAAAAAAAAAAAAAAAAAAA");
 			}*/
	        
	        // Mute set
			if(muteState[i])
				btnMute[i].setImageResource(R.drawable.mute_o);
			else
				btnMute[i].setImageResource(R.drawable.mute_x);
			
			// Reverb set
			if(reverbState[i])
				btnReverb[i].setImageResource(R.drawable.reverb_o);
			else
				btnReverb[i].setImageResource(R.drawable.reverb_x);
			
			NativeAudio.setMuteUriAudioPlayer(muteState[i], i);
			NativeAudio.enableReverb(reverbState[i], i);
			
			Log.d(TAG, "track = " + i + " / volume = " + millibel + " / mute = " + muteState[i] + " / reverb = " + reverbState[i]);
		}
		Log.d(TAG, "====================================");
	}
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	// ===================================================================================================
	private class MyThread implements Runnable {
		// Debug
		private static final	String	TAG	= "MyThread";
		
		private final 			int 	RUNNING  		= 0;
		private final 			int 	SUSPENDED 	= 1;
		private final 			int 	STOPPED 		= 2;
		private 					int 	state 			= SUSPENDED;
		
		private					Thread	mThread = null;	
		
		/** Constructor **/
		public MyThread() {
			this.mThread = new Thread(this);
		}

		@Override
		public void run() {
			while (true) {
				if (checkState()) {
					mThread = null;
					break;
				}
				
				boolean exceptFlag = false;
				int exceptTrackNum = 999;
				
				if(recState[trackNum] == Constants.STATE_REC_WAIT){
					exceptFlag = true;
					
					NativeAudio.startRecording(trackNum);
					recState[trackNum] = Constants.STATE_REC_ING;
					mHandler.sendEmptyMessage(Constants.REDRAW_BTN_REC);
					btnRec[trackNum].setClickable(false);
				}
				else if(recState[trackNum] == Constants.STATE_REC_ING){
					exceptFlag = false;
					
					recState[trackNum] = Constants.STATE_REC;
					mHandler.sendEmptyMessage(Constants.REDRAW_BTN_REC);
					mHandler.sendEmptyMessage(Constants.DRAW_WAVEFORM);
					
					for (int i = 0; i < Constants.ALL_TRACK; i++){
						btnRec[i].setClickable(true);
					}
				}
				
				if(exceptFlag)
					exceptTrackNum = trackNum;
				else
					exceptTrackNum = 999;

				Log.d(TAG, "exceptTrackNum = " + exceptTrackNum);
				
				NativeAudio.selectClip(CLIP_PLAYBACK, 1, exceptTrackNum);
				mHandler.sendEmptyMessage(Constants.CHK_FINISH_RECORD);
				
				applyVolumeAndMuteAndReverb();
				
				try {
					Thread.sleep(Constants.recTime);
				} catch (InterruptedException ie) {
					ie.printStackTrace();
				}				
				
			}
		}
		
		/** User Define Method **/
		private synchronized void setState(int state) {
			this.state = state;
			
			if (this.state == RUNNING) {
				notify();
			} else {
				mThread.interrupt();
			}
		}
		
		private synchronized boolean checkState() {		
			while (state == SUSPENDED) {
				try {
					wait();
				} catch (InterruptedException ie) {
					ie.printStackTrace();
				}
			}
			
			return state == STOPPED;
		}

		public void start() {		
			mThread.start();
			
			setState(RUNNING);
		}
		
		public void stop() {
			setState(STOPPED);
		}
	}
	
	// End of MyThread
}

// End of LoopLoopActivity