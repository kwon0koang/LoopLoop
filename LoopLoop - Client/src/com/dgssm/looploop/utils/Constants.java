package com.dgssm.looploop.utils;

import android.os.Environment;

public class Constants {
	public static int recTime = 5000;
	
	public static final int ALL_TRACK = 5;
	
	// repeat
	public static final int REPEAT = 100;
	
	// record state
	public static final int STATE_REC = 1000;
	public static final int STATE_REC_WAIT = 1001;
	public static final int STATE_REC_ING = 1002;
	
	// handle msg
    public static final int CHK_FINISH_RECORD = 10001;
    public static final int CHK_FINISH_ENCODE = 10002;
    public static final int CHK_FINISH_PLAY = 10003;
    public static final int REDRAW_BTN_REC = 10004;
    public static final int DRAW_WAVEFORM = 10005;
    
    public static final int MSG_TRACK_NUMBER = 20;
    public static final int MSG_TRACK_BUFFER = 21;
    
    public static final String 	IP		= "210.118.75.122";
    public static final int		PORT	= 9000;
    
    // File path
    public static final String[] M4A_PATHS = { Environment.getExternalStorageDirectory() + "/rawFile1.m4a",
    										   Environment.getExternalStorageDirectory() + "/rawFile2.m4a",
    										   Environment.getExternalStorageDirectory() + "/rawFile3.m4a",
    										   Environment.getExternalStorageDirectory() + "/rawFile4.m4a",
    										   Environment.getExternalStorageDirectory() + "/rawFile5.m4a" };
    public static final String[] PCM_PATHS = { Environment.getExternalStorageDirectory() + "/rawFile1.pcm",
    										   Environment.getExternalStorageDirectory() + "/rawFile2.pcm",
    										   Environment.getExternalStorageDirectory() + "/rawFile3.pcm",
    										   Environment.getExternalStorageDirectory() + "/rawFile4.pcm",
    										   Environment.getExternalStorageDirectory() + "/rawFile5.pcm" };

    
}

// End of Constants