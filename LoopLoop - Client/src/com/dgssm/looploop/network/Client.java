package com.dgssm.looploop.network;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.IOException;
import java.net.InetAddress;
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.ShortBuffer;

import android.content.Context;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.widget.Toast;

import com.dgssm.looploop.nativeaudio.NativeAudio;
import com.dgssm.looploop.utils.Constants;

public class Client extends AsyncTask<Object, Integer, Integer> {
	// Debug
	private static final 	String 					TAG 		= "Client";
	
	// Context
	private					Context					mContext	= null;
	
	private					Handler					mHandler	= null;
	
	// Network
	private 				InetAddress 			mServerAddr 	= null;
	private					Socket 					mSocket 		= null;
	
	// Stream
	private					BufferedInputStream 	mBIS		= null; 
	private					BufferedOutputStream 	mBOS		= null;
	private					int						mTrackNumber = -1;
	private					int						mBufferSize = 0;
	
	/** Constructor **/
	public Client(Context context, Handler handler) {
		this.mContext = context;
		this.mHandler = handler;
	}
	
	/** Override Methods **/	
	@Override
	protected Integer doInBackground(Object... params) {
		try {
			Log.d(TAG, "Connecting...");
			
			mServerAddr = InetAddress.getByName(Constants.IP);
			mSocket 	= new Socket(mServerAddr, Constants.PORT);
			mBIS 		= new BufferedInputStream(mSocket.getInputStream());
			mBOS 		= new BufferedOutputStream(mSocket.getOutputStream());
			
			int number = -1;
			
			while (true) {
				if (mTrackNumber == -1) {
					mTrackNumber = read();
					
					Message msg = mHandler.obtainMessage();
					msg.what = Constants.MSG_TRACK_NUMBER;
					msg.arg1 = mTrackNumber;
					mHandler.sendMessage(msg);
				} else {
					int recTime = read();
					
					if (recTime > 0) {
						int len = 0;
						int size = recTime * 16 * 1000 * 2;
						byte[] data = new byte[size];
						
						do {
							Log.e(TAG, "------------------------ len : " + len);
						} while ((len += mBIS.read(data)) != size);
						
						short[] shorts = new short[size / 2];
						ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN).asShortBuffer().get(shorts);
						
						Message msg = mHandler.obtainMessage();
						msg.what = Constants.MSG_TRACK_BUFFER;
						msg.arg1 = 2;
						msg.obj = shorts;
						mHandler.sendMessage(msg);
					}
				}
			}
		} catch (UnknownHostException uhe) {
			uhe.printStackTrace();
		} catch (IOException ioe) {
			ioe.printStackTrace();
		}
		
		return null;
	}
	
	@Override
	protected void onPostExecute(Integer result) {
		super.onPostExecute(result);
	}

	/** User Define Methods **/
	private int read() throws IOException {
		int data = mBIS.read();
		
		return data;
	}
	
	private byte[] read(int size) throws IOException {
		byte[] data = new byte[1024];
		size = mBIS.read(data);
		
		return data;
	}
	
	public synchronized void write(int data) throws Exception {
		if(mSocket == null) return;
		
		Log.e(TAG, "-------------------------------------------- data " + data);
		
		synchronized (mBOS) {
			mBOS.write(data);
			mBOS.flush();
		}
	}
	
	public synchronized void write(byte[] data, int size) throws Exception {
		if(mSocket == null) return;
		
		synchronized (mBOS) {
			mBOS.write(data, 0, size);
			mBOS.flush();
		}
	}
	
	public void stop() {
		this.cancel(true);
		
		if (isCancelled() == true) {
			Log.e(TAG, "Task is cancelled");
			
			try {
				if (mSocket != null) {
					mTrackNumber = -1;
					mSocket.close();
					mSocket = null;
					
					Log.d(TAG, "socket close success");
				}
			} catch (IOException ioe) {
				ioe.printStackTrace();
			}
		}
	}
}

// End of TCPClient