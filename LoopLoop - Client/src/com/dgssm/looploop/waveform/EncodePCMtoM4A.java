package com.dgssm.looploop.waveform;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.ByteBuffer;

import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.media.MediaMuxer;
import android.os.Environment;
import android.util.Log;

public class EncodePCMtoM4A {

	private static final String TAG = "LOG";
	
	private static String AUDIO_RECORDING_FILE_NAME = Environment.getExternalStorageDirectory() + "/" + "rawFile.pcm"; // Input PCM file
	private static String COMPRESSED_AUDIO_FILE_NAME = Environment.getExternalStorageDirectory() + "/" + "rawFile.m4a"; // Output MP4 file
	private static final String COMPRESSED_AUDIO_FILE_MIME_TYPE = "audio/mp4a-latm";
	private static final int COMPRESSED_AUDIO_FILE_BIT_RATE = 128000; // 128kbps
	private static final int SAMPLING_RATE = 16000;
	private static final int CODEC_TIMEOUT_IN_MS = 5000;
	private static final int BUFFER_SIZE = 88200;
	
	private static boolean mStop;
	
	public static boolean chkFinishEncode = false;
	
	public static void myEncode(int trackNum){
		try {
			if(trackNum == 0){
				Log.d(TAG, "trackNum = " + trackNum);
				AUDIO_RECORDING_FILE_NAME = Environment.getExternalStorageDirectory() + "/" + "rawFile1.pcm"; // Input PCM file
				COMPRESSED_AUDIO_FILE_NAME = Environment.getExternalStorageDirectory() + "/" + "rawFile1.m4a"; // Output MP4 file
			}
			else if(trackNum == 1){
				Log.d(TAG, "trackNum = " + trackNum);
				AUDIO_RECORDING_FILE_NAME = Environment.getExternalStorageDirectory() + "/" + "rawFile2.pcm"; // Input PCM file
				COMPRESSED_AUDIO_FILE_NAME = Environment.getExternalStorageDirectory()+ "/" + "rawFile2.m4a"; // Output MP4 file
			}
			else if(trackNum == 2){
				Log.d(TAG, "trackNum = " + trackNum);
				AUDIO_RECORDING_FILE_NAME = Environment.getExternalStorageDirectory() + "/" + "rawFile3.pcm"; // Input PCM file
				COMPRESSED_AUDIO_FILE_NAME = Environment.getExternalStorageDirectory() + "/" + "rawFile3.m4a"; // Output MP4 file
			}
			else if(trackNum == 3){
				Log.d(TAG, "trackNum = " + trackNum);
				AUDIO_RECORDING_FILE_NAME = Environment.getExternalStorageDirectory() + "/" + "rawFile4.pcm"; // Input PCM file
				COMPRESSED_AUDIO_FILE_NAME = Environment.getExternalStorageDirectory() + "/" + "rawFile4.m4a"; // Output MP4 file
			}
			else if(trackNum == 4){
				Log.d(TAG, "trackNum = " + trackNum);
				AUDIO_RECORDING_FILE_NAME = Environment.getExternalStorageDirectory() + "/" + "rawFile5.pcm"; // Input PCM file
				COMPRESSED_AUDIO_FILE_NAME = Environment.getExternalStorageDirectory() + "/" + "rawFile5.m4a"; // Output MP4 file
			}
			
			
	        String filePath = AUDIO_RECORDING_FILE_NAME;
	        File inputFile = new File(filePath);
	        FileInputStream fis = new FileInputStream(inputFile);

	        File outputFile = new File(COMPRESSED_AUDIO_FILE_NAME);
	        if (outputFile.exists()) outputFile.delete();

	        MediaMuxer mux = new MediaMuxer(outputFile.getAbsolutePath(), MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4);

	        MediaFormat outputFormat = MediaFormat.createAudioFormat(COMPRESSED_AUDIO_FILE_MIME_TYPE, SAMPLING_RATE, 1);
	        outputFormat.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);
	        outputFormat.setInteger(MediaFormat.KEY_BIT_RATE, COMPRESSED_AUDIO_FILE_BIT_RATE);

	        MediaCodec codec = MediaCodec.createEncoderByType(COMPRESSED_AUDIO_FILE_MIME_TYPE);
	        codec.configure(outputFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
	        codec.start();

	        
	        ByteBuffer[] codecInputBuffers = codec.getInputBuffers();		 // Note: Array of buffers
	        ByteBuffer[] codecOutputBuffers = codec.getOutputBuffers();

	        MediaCodec.BufferInfo outBuffInfo = new MediaCodec.BufferInfo();

	        byte[] tempBuffer = new byte[BUFFER_SIZE];
	        boolean hasMoreData = true;
	        double presentationTimeUs = 0;
	        int audioTrackIdx = 0;
	        int totalBytesRead = 0; 
	        int percentComplete;

	        do {
	            int inputBufIndex = 0;
	            while (inputBufIndex != -1 && hasMoreData) {
	                inputBufIndex = codec.dequeueInputBuffer(CODEC_TIMEOUT_IN_MS);

	                if (inputBufIndex >= 0) {
	                    ByteBuffer dstBuf = codecInputBuffers[inputBufIndex];
	                    dstBuf.clear();

	                    int bytesRead = fis.read(tempBuffer, 0, dstBuf.limit());
	                    if (bytesRead == -1) { // -1 implies EOS
	                        hasMoreData = false;
	                        codec.queueInputBuffer(inputBufIndex, 0, 0, (long) presentationTimeUs, MediaCodec.BUFFER_FLAG_END_OF_STREAM);
	                    } else {
	                        totalBytesRead += bytesRead;
	                        dstBuf.put(tempBuffer, 0, bytesRead);
	                        codec.queueInputBuffer(inputBufIndex, 0, bytesRead, (long) presentationTimeUs, 0);
	                        presentationTimeUs = 1000000l * (totalBytesRead / 2) / SAMPLING_RATE;
	                    }
	                }
	            }

	            // Drain audio
	            int outputBufIndex = 0;
	            while (outputBufIndex != MediaCodec.INFO_TRY_AGAIN_LATER) {
	                outputBufIndex = codec.dequeueOutputBuffer(outBuffInfo, CODEC_TIMEOUT_IN_MS);
	                if (outputBufIndex >= 0) {
	                    ByteBuffer encodedData = codecOutputBuffers[outputBufIndex];
	                    encodedData.position(outBuffInfo.offset);
	                    encodedData.limit(outBuffInfo.offset + outBuffInfo.size);

	                    if ((outBuffInfo.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0 && outBuffInfo.size != 0) {
	                        codec.releaseOutputBuffer(outputBufIndex, false);
	                    } else {
	                        mux.writeSampleData(audioTrackIdx, codecOutputBuffers[outputBufIndex], outBuffInfo);
	                        codec.releaseOutputBuffer(outputBufIndex, false);
	                    }
	                } else if (outputBufIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
	                    outputFormat = codec.getOutputFormat();
	                    Log.v(TAG, "Output format changed - " + outputFormat);
	                    audioTrackIdx = mux.addTrack(outputFormat);
	                    mux.start();
	                } else if (outputBufIndex == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
	                    Log.e(TAG, "Output buffers changed during encode!");
	                } else if (outputBufIndex == MediaCodec.INFO_TRY_AGAIN_LATER) {
	                    // NO OP
	                } else {
	                    Log.e(TAG, "Unknown return code from dequeueOutputBuffer - " + outputBufIndex);
	                }
	            }
	            percentComplete = (int) Math.round(((float) totalBytesRead / (float) inputFile.length()) * 100.0);
	            Log.v(TAG, "Conversion % - " + percentComplete);
	        } while (outBuffInfo.flags != MediaCodec.BUFFER_FLAG_END_OF_STREAM && !mStop);

	        fis.close();
	        mux.stop();
	        mux.release();
	        Log.v(TAG, "Compression done ...");
	    } catch (FileNotFoundException e) {
	        Log.e(TAG, "File not found!", e);
	    } catch (IOException e) {
	        Log.e(TAG, "IO exception!", e);
	    }
		chkFinishEncode = true;
	    // Notify UI thread...
	}
	
	
	
}
