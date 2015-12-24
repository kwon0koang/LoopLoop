/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.dgssm.looploop.nativeaudio;

//import android.util.Log;

public class NativeAudio {

    /** Native methods, implemented in jni folder 
     * @return */
	public static native boolean chkFinishRecord();
	public static native boolean chkFinishPlay();
	public static native void decideRecordTime(int recTime);
    public static native void createEngine();
    public static native void createBufferQueueAudioPlayer();
    
    // true == PLAYING, false == PAUSED
    public static native void setVolumeUriAudioPlayer(int millibel, int trackNum);
    public static native boolean selectClip(int which, int count, int exceptTrackNum);
    public static native void setMuteUriAudioPlayer(boolean enable, int index);
    public static native boolean enableReverb(boolean enabled, int index);
    public static native boolean createAudioRecorder();
    public static native void startRecording(int index);
    public static native void shutdown();
    //public static native int[] waveform(int[] audio);
    
    public static native void setRecorderBuffer(int index, short[] buffer);
    public static native short[] getRecorderBuffer(int index);
    
    /** Load jni .so on initialization */
    static {
         System.loadLibrary("native-audio-jni");
    }

}
