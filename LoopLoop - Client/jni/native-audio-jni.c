/* * Copyright (C) 2010 The Android Open Source Project
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
 *
 */

/* This is a JNI example where we use native methods to play sounds
 * using OpenSL ES. See the corresponding Java source file located at:
 *
 *   src/com/example/nativeaudio/NativeAudio/NativeAudio.java
 */

#include <assert.h>
#include <jni.h>
#include <string.h>
#include <android/log.h>
#include <stdio.h>


// for __android_log_print(ANDROID_LOG_INFO, "YourApp", "formatted message");
// #include <android/log.h>

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

// Track Number
#define TRACK 5

// for log
#define  LOG_TAG    "NativeAudio-JNI"
#define  LOGUNK(...)  __android_log_print(ANDROID_LOG_UNKNOWN,LOG_TAG,__VA_ARGS__)
#define  LOGDEF(...)  __android_log_print(ANDROID_LOG_DEFAULT,LOG_TAG,__VA_ARGS__)
#define  LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,__VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGF(...)  __android_log_print(ANDROID_FATAL_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGS(...)  __android_log_print(ANDROID_SILENT_ERROR,LOG_TAG,__VA_ARGS__)

// pre-recorded sound clips, both are 8 kHz mono 16-bit signed little endian

/*
static const char hello[] =
#include "hello_clip.h"
;

static const char android[] =
#include "android_clip.h"
*/
;

// engine interfaces
static SLObjectItf engineObject[TRACK];
static SLEngineItf engineEngine[TRACK];

// output mix interfaces
static SLObjectItf outputMixObject = NULL;
static SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;

// buffer queue player interfaces
static SLObjectItf bqPlayerObject[TRACK];
static SLPlayItf bqPlayerPlay[TRACK];
static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue[TRACK];
static SLEffectSendItf bqPlayerEffectSend[TRACK];
static SLMuteSoloItf bqPlayerMuteSolo[TRACK];
static SLVolumeItf bqPlayerVolume[TRACK];

// aux effect on the output mix, used by the buffer queue player
static const SLEnvironmentalReverbSettings reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

// recorder interfaces
static SLObjectItf recorderObject[TRACK];
static SLRecordItf recorderRecord[TRACK];
static SLAndroidSimpleBufferQueueItf recorderBufferQueue[TRACK];

// synthesized sawtooth clip
#define SAWTOOTH_FRAMES 8000
static short sawtoothBuffer[SAWTOOTH_FRAMES];

// 5 seconds of recorded audio at 16 kHz mono, 16-bit signed little endian
/*#define RECORDER_FRAMES (16000 * 3)
static short recorderBuffer[TRACK][RECORDER_FRAMES];
static unsigned recorderSize[TRACK] = {0, };
static SLmilliHertz recorderSR[TRACK];*/
// kwon -------------------------------
static int recordTime = 0;
static int RECORDER_FRAMES = 0;
static short *recorderBuffer[TRACK];
static unsigned recorderSize[TRACK] = {0, };
static SLmilliHertz recorderSR[TRACK];

// pointer and size of the next player buffer to enqueue, and number of remaining buffers
static short *nextBuffer[TRACK];
static unsigned nextSize[TRACK];
static int nextCount[TRACK];

FILE* rawFile = NULL;
int bClosing = 0;

static jboolean chkFinishRecordFlag = JNI_FALSE;
static jboolean chkFinishPlayFlag = JNI_FALSE;

// ==================================================================================
// ==================================================================================
// ==================================================================================



// synthesize a mono sawtooth wave and place it into a buffer (called automatically on load)
__attribute__((constructor)) static void onDlOpen(void)
{
    unsigned i;
    for (i = 0; i < SAWTOOTH_FRAMES; ++i) {
        sawtoothBuffer[i] = 32768 - ((i % 100) * 660);
    }
}

//======================================================================================================================
// bqPlayerCallback =========================================================================================================
//======================================================================================================================

// this callback handler is called every time a buffer finishes playing
void bqPlayerCallback_0(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	LOGD("bqPlayerCallback_0");
	if(chkFinishPlayFlag == JNI_TRUE)
		chkFinishPlayFlag = JNI_FALSE;
	else
		chkFinishPlayFlag = JNI_TRUE;


	assert(bq == bqPlayerBufferQueue[0]);
	assert(NULL == context);
	// for streaming playback, replace this test by logic to find and fill the next buffer
	if (--nextCount[0] > 0 && NULL != nextBuffer[0] && 0 != nextSize[0]) {
		SLresult result;
		// enqueue another buffer
		result = (*(bqPlayerBufferQueue[0]))->Enqueue(bqPlayerBufferQueue[0], nextBuffer[0], nextSize[0]);
		// the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
		// which for this code example would indicate a programming error
		assert(SL_RESULT_SUCCESS == result);
		(void)result;
	}
}

void bqPlayerCallback_1(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	LOGD("bqPlayerCallback_1");

	assert(bq == bqPlayerBufferQueue[1]);
	assert(NULL == context);

	// for streaming playback, replace this test by logic to find and fill the next buffer
	if (--nextCount[1] > 0 && NULL != nextBuffer[1] && 0 != nextSize[1]) {
		SLresult result;

		// enqueue another buffer
		result = (*(bqPlayerBufferQueue[1]))->Enqueue(bqPlayerBufferQueue[1], nextBuffer[1], nextSize[1]);
		// the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
		// which for this code example would indicate a programming error
		assert(SL_RESULT_SUCCESS == result);
		(void)result;
	}
}

void bqPlayerCallback_2(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	LOGD("bqPlayerCallback_2");

	assert(bq == bqPlayerBufferQueue[2]);
	assert(NULL == context);

	// for streaming playback, replace this test by logic to find and fill the next buffer
	if (--nextCount[2] > 0 && NULL != nextBuffer[2] && 0 != nextSize[2]) {
		SLresult result;

		// enqueue another buffer
		result = (*(bqPlayerBufferQueue[2]))->Enqueue(bqPlayerBufferQueue[2], nextBuffer[2], nextSize[2]);
		// the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
		// which for this code example would indicate a programming error
		assert(SL_RESULT_SUCCESS == result);
		(void)result;
	}
}

void bqPlayerCallback_3(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	LOGD("bqPlayerCallback_3");

	assert(bq == bqPlayerBufferQueue[3]);
	assert(NULL == context);

	// for streaming playback, replace this test by logic to find and fill the next buffer
	if (--nextCount[3] > 0 && NULL != nextBuffer[3] && 0 != nextSize[3]) {
		SLresult result;

		// enqueue another buffer
		result = (*(bqPlayerBufferQueue[3]))->Enqueue(bqPlayerBufferQueue[3], nextBuffer[3], nextSize[3]);
		// the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
		// which for this code example would indicate a programming error
		assert(SL_RESULT_SUCCESS == result);
		(void)result;
	}
}

void bqPlayerCallback_4(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	LOGD("bqPlayerCallback_4");

	assert(bq == bqPlayerBufferQueue[4]);
	assert(NULL == context);

	// for streaming playback, replace this test by logic to find and fill the next buffer
	if (--nextCount[4] > 0 && NULL != nextBuffer[4] && 0 != nextSize[4]) {
		SLresult result;

		// enqueue another buffer
		result = (*(bqPlayerBufferQueue[4]))->Enqueue(bqPlayerBufferQueue[4], nextBuffer[4], nextSize[4]);
		// the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
		// which for this code example would indicate a programming error
		assert(SL_RESULT_SUCCESS == result);
		(void)result;
	}
}

//======================================================================================================================
// bqRecorderCallback =======================================================================================================
//======================================================================================================================

// this callback handler is called every time a buffer finishes recording
void bqRecorderCallback_0(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	LOGD("bqRecorderCallback_0");

    assert(bq == recorderBufferQueue[0]);
    assert(NULL == context);
    // for streaming recording, here we would call Enqueue to give recorder the next buffer to fill
    // but instead, this is a one-time buffer so we stop recording
    SLresult result;
    result = (*(recorderRecord[0]))->SetRecordState(recorderRecord[0], SL_RECORDSTATE_STOPPED);
    if (SL_RESULT_SUCCESS == result) {
        recorderSize[0] = RECORDER_FRAMES * sizeof(short);
        recorderSR[0] = SL_SAMPLINGRATE_16;
    }
    chkFinishRecordFlag = JNI_TRUE;

	fwrite(recorderBuffer[0], RECORDER_FRAMES * sizeof(short), 1, rawFile);

	fclose(rawFile);
	rawFile = NULL;
}

void bqRecorderCallback_1(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	LOGD("bqRecorderCallback_1");

    assert(bq == recorderBufferQueue[1]);
    assert(NULL == context);
    // for streaming recording, here we would call Enqueue to give recorder the next buffer to fill
    // but instead, this is a one-time buffer so we stop recording
    SLresult result;
    result = (*(recorderRecord[1]))->SetRecordState(recorderRecord[1], SL_RECORDSTATE_STOPPED);
    if (SL_RESULT_SUCCESS == result) {
        recorderSize[1] = RECORDER_FRAMES * sizeof(short);
        recorderSR[1] = SL_SAMPLINGRATE_16;
    }
    chkFinishRecordFlag = JNI_TRUE;

    fwrite(recorderBuffer[1], RECORDER_FRAMES * sizeof(short), 1, rawFile);

	fclose(rawFile);
	rawFile = NULL;
}

void bqRecorderCallback_2(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	LOGD("bqRecorderCallback_2");

    assert(bq == recorderBufferQueue[2]);
    assert(NULL == context);
    // for streaming recording, here we would call Enqueue to give recorder the next buffer to fill
    // but instead, this is a one-time buffer so we stop recording
    SLresult result;
    result = (*(recorderRecord[2]))->SetRecordState(recorderRecord[2], SL_RECORDSTATE_STOPPED);
    if (SL_RESULT_SUCCESS == result) {
        recorderSize[2] = RECORDER_FRAMES * sizeof(short);
        recorderSR[2] = SL_SAMPLINGRATE_16;
    }
    chkFinishRecordFlag = JNI_TRUE;

    fwrite(recorderBuffer[2], RECORDER_FRAMES * sizeof(short), 1, rawFile);

	fclose(rawFile);
	rawFile = NULL;
}

void bqRecorderCallback_3(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	LOGD("bqRecorderCallback_3");

    assert(bq == recorderBufferQueue[3]);
    assert(NULL == context);
    // for streaming recording, here we would call Enqueue to give recorder the next buffer to fill
    // but instead, this is a one-time buffer so we stop recording
    SLresult result;
    result = (*(recorderRecord[3]))->SetRecordState(recorderRecord[3], SL_RECORDSTATE_STOPPED);
    if (SL_RESULT_SUCCESS == result) {
        recorderSize[3] = RECORDER_FRAMES * sizeof(short);
        recorderSR[3] = SL_SAMPLINGRATE_16;
    }
    chkFinishRecordFlag = JNI_TRUE;

    fwrite(recorderBuffer[3], RECORDER_FRAMES * sizeof(short), 1, rawFile);

	fclose(rawFile);
	rawFile = NULL;
}

void bqRecorderCallback_4(SLAndroidSimpleBufferQueueItf bq, void *context)
{
	LOGD("bqRecorderCallback_4");

    assert(bq == recorderBufferQueue[4]);
    assert(NULL == context);
    // for streaming recording, here we would call Enqueue to give recorder the next buffer to fill
    // but instead, this is a one-time buffer so we stop recording
    SLresult result;
    result = (*(recorderRecord[4]))->SetRecordState(recorderRecord[4], SL_RECORDSTATE_STOPPED);
    if (SL_RESULT_SUCCESS == result) {
        recorderSize[4] = RECORDER_FRAMES * sizeof(short);
        recorderSR[4] = SL_SAMPLINGRATE_16;
    }
    chkFinishRecordFlag = JNI_TRUE;

    fwrite(recorderBuffer[4], RECORDER_FRAMES * sizeof(short), 1, rawFile);

	fclose(rawFile);
	rawFile = NULL;
}

// ========================================================================================================
// ========================================================================================================
// JNI FUNCTION =============================================================================================
// ========================================================================================================
// ========================================================================================================

/*jboolean changeChkFinishPlayFlag(jboolean *chk)
{
	jboolean tmp = *chk;
	*chk = JNI_FALSE;

	return tmp;
}*/


// 셋도 하자
void Java_com_dgssm_looploop_nativeaudio_NativeAudio_setRecorderBuffer(JNIEnv* env, jclass clazz, jint trackNum, short *buffer) {
	LOGD("---------------------------------------------------------------------- track %d", trackNum);
	/*int idx=0;
	for(idx=0; idx<100; idx++)
		LOGD("%d", buffer[idx]);*/
//	(*env)->SetShortArrayRegion(env, recorderBuffer[2], 0 , RECORDER_FRAMES, recorderBuffer[0]);
	SLresult result;

	// recorderBuffer[2] = buffer;

	// in case already recording, stop recording and clear buffer queue
	result = (*(recorderRecord[4]))->SetRecordState(recorderRecord[4], SL_RECORDSTATE_STOPPED);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;
	result = (*(recorderBufferQueue[4]))->Clear(recorderBufferQueue[4]);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// the buffer is not valid for playback yet
	recorderSize[4] = 0;

	// enqueue an empty buffer to be filled by the recorder
	// (for streaming recording, we would enqueue at least 2 empty buffers to start things off)
	result = (*(recorderBufferQueue[4]))->Enqueue(recorderBufferQueue[4], recorderBuffer[4], RECORDER_FRAMES * sizeof(short));
	// the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
	// which for this code example would indicate a programming error
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// start recording
	result = (*(recorderRecord[4]))->SetRecordState(recorderRecord[4], SL_RECORDSTATE_RECORDING);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	chkFinishRecordFlag = JNI_FALSE;
	bClosing = 0;

	LOGD("fopen(\"/sdcard/rawFile5.pcm\", \"wb\");");
	rawFile = fopen("/sdcard/rawFile5.pcm", "wb");

//	    if(index == 0){
//	    	LOGD("fopen(\"/sdcard/rawFile1.pcm\", \"wb\");");
//	    	rawFile = fopen("/sdcard/rawFile1.pcm", "wb");
//	    }
//	    else if(index == 1){
//	    	LOGD("fopen(\"/sdcard/rawFile2.pcm\", \"wb\");");
//	        rawFile = fopen("/sdcard/rawFile2.pcm", "wb");
//	    }
//	    else if(index == 2){
//	    	LOGD("fopen(\"/sdcard/rawFile3.pcm\", \"wb\");");
//	        rawFile = fopen("/sdcard/rawFile3.pcm", "wb");
//	    }
//	    else if(index == 3){
//	    	LOGD("fopen(\"/sdcard/rawFile4.pcm\", \"wb\");");
//	        rawFile = fopen("/sdcard/rawFile4.pcm", "wb");
//	    }
//	    else if(index == 4){
//	    	LOGD("fopen(\"/sdcard/rawFile5.pcm\", \"wb\");");
//	        rawFile = fopen("/sdcard/rawFile5.pcm", "wb");
//	    }

	LOGD("---------------------------------------------------------------------- track %d", trackNum);
}

// 겟도 하자
jshortArray Java_com_dgssm_looploop_nativeaudio_NativeAudio_getRecorderBuffer(JNIEnv* env, jclass clazz, jint trackNum) {
	jshortArray outJNIArray = (*env)->NewShortArray(env, RECORDER_FRAMES);		// allocate
	(*env)->SetShortArrayRegion(env, outJNIArray, 0 , RECORDER_FRAMES, recorderBuffer[trackNum]);		// copy

	return outJNIArray;
}

jboolean Java_com_dgssm_looploop_nativeaudio_NativeAudio_chkFinishRecord(JNIEnv* env, jclass clazz) {
	return chkFinishRecordFlag;
}

jboolean Java_com_dgssm_looploop_nativeaudio_NativeAudio_chkFinishPlay(JNIEnv* env, jclass clazz) {
	//return changeChkFinishPlayFlag(&chkFinishPlayFlag);
	return chkFinishPlayFlag;
}

void Java_com_dgssm_looploop_nativeaudio_NativeAudio_decideRecordTime(JNIEnv* env, jclass clazz, jint time)
{
	recordTime = time;
	RECORDER_FRAMES = (16 * time);
	int i=0;
	for(i = 0; i < TRACK; i++){
		recorderSize[i] = RECORDER_FRAMES * sizeof(short);
	}
	LOGD("recordTime = %d, RECORDER_FRAMES = %d, recorderSize = %d", recordTime, RECORDER_FRAMES, (RECORDER_FRAMES * sizeof(short)));

	for(i = 0; i < TRACK; i++){
		recorderBuffer[i] = (short  *)malloc( recorderSize[i] );
	}
}

// create the engine and output mix objects
void Java_com_dgssm_looploop_nativeaudio_NativeAudio_createEngine(JNIEnv* env, jclass clazz)
{
    SLresult result;
    unsigned i;

    // create engine
    for (i = 0; i < TRACK; i++) {
		result = slCreateEngine(&engineObject[i], 0, NULL, 0, NULL, NULL);
		assert(SL_RESULT_SUCCESS == result);
		(void)result;

		// realize the engine
		result = (*(engineObject[i]))->Realize(engineObject[i], SL_BOOLEAN_FALSE);
		assert(SL_RESULT_SUCCESS == result);
		(void)result;

		// get the engine interface, which is needed in order to create other objects
		result = (*(engineObject[i]))->GetInterface(engineObject[i], SL_IID_ENGINE, &engineEngine[i]);
		assert(SL_RESULT_SUCCESS == result);
		(void)result;

		// create output mix, with environmental reverb specified as a non-required interface
		const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
		const SLboolean req[1] = {SL_BOOLEAN_FALSE};
		result = (*(engineEngine[i]))->CreateOutputMix(engineEngine[i], &outputMixObject, 1, ids, req);
		assert(SL_RESULT_SUCCESS == result);
		(void)result;
    }

    // realize the output mix
	result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// get the environmental reverb interface
	// this could fail if the environmental reverb effect is not available,
	// either because the feature is not present, excessive CPU load, or
	// the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
	result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB, &outputMixEnvironmentalReverb);
	if (SL_RESULT_SUCCESS == result) {
		result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
				outputMixEnvironmentalReverb, &reverbSettings);
		(void)result;
	}
}

// create buffer queue audio player
void Java_com_dgssm_looploop_nativeaudio_NativeAudio_createBufferQueueAudioPlayer(JNIEnv* env, jclass clazz)
{
    SLresult result;

    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq[TRACK] = {{SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2},
    														  {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2},
														      {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2},
														      {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2},
    														  {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2}};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_8,
    							   SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
								   SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource audioSrc[TRACK] = {{&loc_bufq[0], &format_pcm},
    								{&loc_bufq[1], &format_pcm},
									{&loc_bufq[2], &format_pcm},
									{&loc_bufq[3], &format_pcm},
    								{&loc_bufq[4], &format_pcm}};

    // configure audio sink
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    // create audio player
    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, /*SL_IID_MUTESOLO,*/ SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, /*SL_BOOLEAN_TRUE,*/ SL_BOOLEAN_TRUE};

    unsigned int i;
    for (i = 0; i < TRACK; i++) {
    	result = (*(engineEngine[i]))->CreateAudioPlayer(engineEngine[i], &bqPlayerObject[i], &audioSrc[i], &audioSnk, 3, ids, req);
		assert(SL_RESULT_SUCCESS == result);
		(void)result;

		// realize the player
		result = (*(bqPlayerObject[i]))->Realize(bqPlayerObject[i], SL_BOOLEAN_FALSE);
		assert(SL_RESULT_SUCCESS == result);
		(void)result;

		// get the play interface
		result = (*(bqPlayerObject[i]))->GetInterface(bqPlayerObject[i], SL_IID_PLAY, &bqPlayerPlay[i]);
		assert(SL_RESULT_SUCCESS == result);
		(void)result;

		// get the buffer queue interface
		result = (*(bqPlayerObject[i]))->GetInterface(bqPlayerObject[i], SL_IID_BUFFERQUEUE, &bqPlayerBufferQueue[i]);
		assert(SL_RESULT_SUCCESS == result);
		(void)result;

		// register callback on the buffer queue
		switch (i) {
			case 0 :
				result = (*(bqPlayerBufferQueue[i]))->RegisterCallback(bqPlayerBufferQueue[i], bqPlayerCallback_0, NULL);
				assert(SL_RESULT_SUCCESS == result);
				(void)result;
				break;
			case 1 :
				result = (*(bqPlayerBufferQueue[i]))->RegisterCallback(bqPlayerBufferQueue[i], bqPlayerCallback_1, NULL);
				assert(SL_RESULT_SUCCESS == result);
				(void)result;
				break;
			case 2 :
				result = (*(bqPlayerBufferQueue[i]))->RegisterCallback(bqPlayerBufferQueue[i], bqPlayerCallback_2, NULL);
				assert(SL_RESULT_SUCCESS == result);
				(void)result;
				break;
			case 3 :
				result = (*(bqPlayerBufferQueue[i]))->RegisterCallback(bqPlayerBufferQueue[i], bqPlayerCallback_3, NULL);
				assert(SL_RESULT_SUCCESS == result);
				(void)result;
				break;
			case 4 :
				result = (*(bqPlayerBufferQueue[i]))->RegisterCallback(bqPlayerBufferQueue[i], bqPlayerCallback_4, NULL);
				assert(SL_RESULT_SUCCESS == result);
				(void)result;
				break;
		}

		// get the effect send interface
		result = (*(bqPlayerObject[i]))->GetInterface(bqPlayerObject[i], SL_IID_EFFECTSEND, &bqPlayerEffectSend[i]);
		assert(SL_RESULT_SUCCESS == result);
		(void)result;

	#if 0   // mute/solo is not supported for sources that are known to be mono, as this is
		// get the mute/solo interface
		result = (*(bqPlayerObject[i]))->GetInterface(bqPlayerObject[i], SL_IID_MUTESOLO, &bqPlayerMuteSolo[i]);
		assert(SL_RESULT_SUCCESS == result);
		(void)result;
	#endif
		// get the volume interface
		result = (*(bqPlayerObject[i]))->GetInterface(bqPlayerObject[i], SL_IID_VOLUME, &bqPlayerVolume[i]);
		assert(SL_RESULT_SUCCESS == result);
		(void)result;

		// set the player's state to playing
		result = (*(bqPlayerPlay[i]))->SetPlayState(bqPlayerPlay[i], SL_PLAYSTATE_PLAYING);
		assert(SL_RESULT_SUCCESS == result);
		(void)result;
    }
}

// expose the mute/solo APIs to Java for one of the 3 players
//static SLMuteSoloItf getMuteSolo()
//{
//    if (uriPlayerMuteSolo != NULL)
//        return uriPlayerMuteSolo;
//    else if (fdPlayerMuteSolo != NULL)
//        return fdPlayerMuteSolo;
//    else
//	return bqPlayerMuteSolo;
//}

// expose the volume APIs to Java for one of the 3 players
static SLVolumeItf getVolume(int trackNum)
{
    /*if (uriPlayerVolume != NULL)
        return uriPlayerVolume;
    else if (fdPlayerVolume != NULL)
        return fdPlayerVolume;
    else*/
	return bqPlayerVolume[trackNum];
}

void Java_com_dgssm_looploop_nativeaudio_NativeAudio_setVolumeUriAudioPlayer(JNIEnv* env, jclass clazz, jint millibel, jint jtrackNum)
{
    SLresult result;
    SLVolumeItf volumeItf = getVolume((int)jtrackNum);
    if (NULL != volumeItf) {
        result = (*volumeItf)->SetVolumeLevel(volumeItf, millibel);
        assert(SL_RESULT_SUCCESS == result);
        (void)result;
    }
}

void Java_com_dgssm_looploop_nativeaudio_NativeAudio_setMuteUriAudioPlayer(JNIEnv* env, jclass clazz, jboolean mute, jint index)
{
    SLresult result;
    SLVolumeItf volumeItf = getVolume(index);
    if (NULL != volumeItf) {
        result = (*volumeItf)->SetMute(volumeItf, mute);
        assert(SL_RESULT_SUCCESS == result);
        (void)result;
    }
}

/*void Java_com_dgssm_looploop_nativeaudio_NativeAudio_setStereoPositionUriAudioPlayer(JNIEnv* env, jclass clazz, jint permille, jint jtrackNum)
{
	LOGD("permile = %d / jtrackNum = %d", (int)permille, (int)jtrackNum);
    SLresult result;
    SLVolumeItf volumeItf = getVolume(jtrackNum);
    if (NULL != volumeItf) {
        result = (*volumeItf)->SetStereoPosition(volumeItf, permille);
        assert(SL_RESULT_SUCCESS == result);
        (void)result;
    }
}*/

// enable reverb on the buffer queue player
jboolean Java_com_dgssm_looploop_nativeaudio_NativeAudio_enableReverb(JNIEnv* env, jclass clazz, jboolean enabled, jint index)
{
    SLresult result;

    // we might not have been able to add environmental reverb to the output mix
    if (NULL == outputMixEnvironmentalReverb) {
        return JNI_FALSE;
    }

    result = (*(bqPlayerEffectSend[index]))->EnableEffectSend(bqPlayerEffectSend[index], outputMixEnvironmentalReverb, (SLboolean) enabled, (SLmillibel) 0);
    // and even if environmental reverb was present, it might no longer be available
    if (SL_RESULT_SUCCESS != result) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}


// select the desired clip and play count, and enqueue the first buffer if idle
jboolean Java_com_dgssm_looploop_nativeaudio_NativeAudio_selectClip(JNIEnv* env, jclass clazz, jint which, jint count, jint exceptTrackNum)
{
	LOGD("selectClip");
	unsigned int i;

	int tmpWhich = (int)which;

	for (i = 0; i < TRACK; i++) {
		//if(i == (int)exceptTrackNum) continue;
		if(i == (int)exceptTrackNum) {
			which = 0;
		}
		else{
			which = tmpWhich;
		}

		switch (which) {
		case 0:     // CLIP_NONE
			nextBuffer[i] = (short *) NULL;
			nextSize[i] = 0;
			break;
		case 4:     // CLIP_PLAYBACK
			// we recorded at 16 kHz, but are playing buffers at 8 Khz, so do a primitive down-sample
			if (recorderSR[i] == SL_SAMPLINGRATE_16) {
				unsigned j;
				for (j = 0; j < recorderSize[i]; j += 2 * sizeof(short)) {
					recorderBuffer[i][j >> 2] = recorderBuffer[i][j >> 1];
				}
				recorderSR[i] = SL_SAMPLINGRATE_8;
				recorderSize[i] >>= 1;

				nextBuffer[i] = recorderBuffer[i];
				nextSize[i] = recorderSize[i];
			}

			break;
		default:
			nextBuffer[i] = NULL;
			nextSize[i] = 0;
			break;
		}
		nextCount[i] = count;
		if (nextSize[i] > 0) {
			// here we only enqueue one buffer because it is a long clip,
			// but for streaming playback we would typically enqueue at least 2 buffers to start
			SLresult result;

			result = (*(bqPlayerBufferQueue[i]))->Enqueue(bqPlayerBufferQueue[i], nextBuffer[i], nextSize[i]);
			if (SL_RESULT_SUCCESS != result) {
				return JNI_FALSE;
			}
		}
	}

    return JNI_TRUE;
}


// create audio recorder
jboolean Java_com_dgssm_looploop_nativeaudio_NativeAudio_createAudioRecorder(JNIEnv* env, jclass clazz)
{
	LOGD("createAudioRecorder()");
    SLresult result;
    unsigned int i;

    // configure audio source
//    SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
    SLDataLocator_IODevice loc_dev[TRACK] = {{SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL},
    										 {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL},
											 {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL},
											 {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL},
											 {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL}};
//    SLDataSource audioSrc = {&loc_dev, NULL};
    SLDataSource audioSrc[TRACK] = {{&loc_dev[0], NULL},
    								{&loc_dev[1], NULL},
									{&loc_dev[2], NULL},
									{&loc_dev[3], NULL},
									{&loc_dev[4], NULL}};

    // configure audio sink
//    SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataLocator_AndroidSimpleBufferQueue loc_bq[TRACK] = {{SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2},
    														{SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2},
															{SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2},
															{SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2},
															{SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2}};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_16,
    							   SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
								   SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};

//    SLDataSink audioSnk = {&loc_bq, &format_pcm};
    SLDataSink audioSnk[TRACK] = {{&loc_bq[0], &format_pcm},
    							  {&loc_bq[1], &format_pcm},
								  {&loc_bq[2], &format_pcm},
								  {&loc_bq[3], &format_pcm},
								  {&loc_bq[4], &format_pcm}};

    // create audio recorder
    // (requires the RECORD_AUDIO permission)
    const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};

//    result = (*(engineEngine[0]))->CreateAudioRecorder(engineEngine[0], &recorderObject, &audioSrc, &audioSnk, 1, id, req);
//	if (SL_RESULT_SUCCESS != result) {
//		return JNI_FALSE;
//	}
//
//	// realize the audio recorder
//	result = (*recorderObject)->Realize(recorderObject, SL_BOOLEAN_FALSE);
//	if (SL_RESULT_SUCCESS != result) {
//		return JNI_FALSE;
//	}
//
//	// get the record interface
//	result = (*recorderObject)->GetInterface(recorderObject, SL_IID_RECORD, &recorderRecord);
//	assert(SL_RESULT_SUCCESS == result);
//	(void)result;
//
//	// get the buffer queue interface
//	result = (*recorderObject)->GetInterface(recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &recorderBufferQueue);
//	assert(SL_RESULT_SUCCESS == result);
//	(void)result;
//
//    result = (*recorderBufferQueue)->RegisterCallback(recorderBufferQueue, bqRecorderCallback_1, NULL);
//	assert(SL_RESULT_SUCCESS == result);
//	(void)result;

    for (i = 0; i < TRACK; i++) {
		result = (*(engineEngine[i]))->CreateAudioRecorder(engineEngine[i], &recorderObject[i], &audioSrc[i], &audioSnk[i], 1, id, req);
		if (SL_RESULT_SUCCESS != result) {
			return JNI_FALSE;
		}

		// realize the audio recorder
		result = (*(recorderObject[i]))->Realize(recorderObject[i], SL_BOOLEAN_FALSE);
		if (SL_RESULT_SUCCESS != result) {
			return JNI_FALSE;
		}

		// get the record interface
		result = (*(recorderObject[i]))->GetInterface(recorderObject[i], SL_IID_RECORD, &recorderRecord[i]);
		assert(SL_RESULT_SUCCESS == result);
		(void)result;

		// get the buffer queue interface
		result = (*(recorderObject[i]))->GetInterface(recorderObject[i], SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &recorderBufferQueue[i]);
		assert(SL_RESULT_SUCCESS == result);
		(void)result;

		// register callback on the buffer queue
		switch (i) {
			case 0 :
				result = (*(recorderBufferQueue[i]))->RegisterCallback(recorderBufferQueue[i], bqRecorderCallback_0, NULL);
				assert(SL_RESULT_SUCCESS == result);
				(void)result;
				break;
			case 1 :
				result = (*(recorderBufferQueue[i]))->RegisterCallback(recorderBufferQueue[i], bqRecorderCallback_1, NULL);
				assert(SL_RESULT_SUCCESS == result);
				(void)result;
				break;
			case 2 :
				result = (*(recorderBufferQueue[i]))->RegisterCallback(recorderBufferQueue[i], bqRecorderCallback_2, NULL);
				assert(SL_RESULT_SUCCESS == result);
				(void)result;
				break;
			case 3 :
				result = (*(recorderBufferQueue[i]))->RegisterCallback(recorderBufferQueue[i], bqRecorderCallback_3, NULL);
				assert(SL_RESULT_SUCCESS == result);
				(void)result;
				break;
			case 4 :
				result = (*(recorderBufferQueue[i]))->RegisterCallback(recorderBufferQueue[i], bqRecorderCallback_4, NULL);
				assert(SL_RESULT_SUCCESS == result);
				(void)result;
				break;
			default : break;
		}
    }

    return JNI_TRUE;
}


// set the recording state for the audio recorder
void Java_com_dgssm_looploop_nativeaudio_NativeAudio_startRecording(JNIEnv* env, jclass clazz, jint index)
{
	LOGD("startRecording()");
    SLresult result;

    // in case already recording, stop recording and clear buffer queue
    result = (*(recorderRecord[index]))->SetRecordState(recorderRecord[index], SL_RECORDSTATE_STOPPED);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
    result = (*(recorderBufferQueue[index]))->Clear(recorderBufferQueue[index]);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // the buffer is not valid for playback yet
    recorderSize[index] = 0;

    // enqueue an empty buffer to be filled by the recorder
    // (for streaming recording, we would enqueue at least 2 empty buffers to start things off)
    result = (*(recorderBufferQueue[index]))->Enqueue(recorderBufferQueue[index], recorderBuffer[index], RECORDER_FRAMES * sizeof(short));
    // the most likely other result is SL_RESULT_BUFFER_INSUFFICIENT,
    // which for this code example would indicate a programming error
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // start recording
    result = (*(recorderRecord[index]))->SetRecordState(recorderRecord[index], SL_RECORDSTATE_RECORDING);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    chkFinishRecordFlag = JNI_FALSE;
    bClosing = 0;
    if(index == 0){
    	LOGD("fopen(\"/sdcard/rawFile1.pcm\", \"wb\");");
    	rawFile = fopen("/sdcard/rawFile1.pcm", "wb");
    }
    else if(index == 1){
    	LOGD("fopen(\"/sdcard/rawFile2.pcm\", \"wb\");");
        rawFile = fopen("/sdcard/rawFile2.pcm", "wb");
    }
    else if(index == 2){
    	LOGD("fopen(\"/sdcard/rawFile3.pcm\", \"wb\");");
        rawFile = fopen("/sdcard/rawFile3.pcm", "wb");
    }
    else if(index == 3){
    	LOGD("fopen(\"/sdcard/rawFile4.pcm\", \"wb\");");
        rawFile = fopen("/sdcard/rawFile4.pcm", "wb");
    }
    else if(index == 4){
    	LOGD("fopen(\"/sdcard/rawFile5.pcm\", \"wb\");");
        rawFile = fopen("/sdcard/rawFile5.pcm", "wb");
    }
}

void Java_com_dgssm_looploop_nativeaudio_NativeAudio_isNULL(JNIEnv* env, jclass clazz, jint index)
{
	if (recorderObject[index] != NULL) {
		recorderBufferQueue[index] = 0;
	}
}

// shut down the native audio system
void Java_com_dgssm_looploop_nativeaudio_NativeAudio_shutdown(JNIEnv* env, jclass clazz)
{
	bClosing = 1;

    unsigned i;

    for (i = 0; i < TRACK; i++) {
		// destroy buffer queue audio player object, and invalidate all associated interfaces
		if (bqPlayerObject[i] != NULL) {
			(*(bqPlayerObject[i]))->Destroy(bqPlayerObject[i]);
			bqPlayerObject[i] = NULL;
			bqPlayerPlay[i] = NULL;
			bqPlayerBufferQueue[i] = NULL;
			bqPlayerEffectSend[i] = NULL;
			bqPlayerMuteSolo[i] = NULL;
			bqPlayerVolume[i] = NULL;
		}

		// destroy audio recorder object, and invalidate all associated interfaces
		if (recorderObject[i] != NULL) {
			(*(recorderObject[i]))->Destroy(recorderObject[i]);
			recorderObject[i] = NULL;
			recorderRecord[i] = NULL;
			recorderBufferQueue[i] = NULL;
		}
    }

	// destroy output mix object, and invalidate all associated interfaces
	if (outputMixObject != NULL) {
		(*outputMixObject)->Destroy(outputMixObject);
		outputMixObject = NULL;
		outputMixEnvironmentalReverb = NULL;
	}

    for (i = 0; i < 4; i++) {
		// destroy engine object, and invalidate all associated interfaces
		if (engineObject[i] != NULL) {
			(*(engineObject[i]))->Destroy(engineObject[i]);
			engineObject[i] = NULL;
			engineEngine[i] = NULL;
		}
    }
}
