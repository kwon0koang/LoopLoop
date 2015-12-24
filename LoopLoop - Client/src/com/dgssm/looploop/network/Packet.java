package com.dgssm.looploop.network;

import org.json.JSONException;
import org.json.JSONObject;

public class Packet implements JsonSerializable {
	private static final	String	NUMBER = "number";
	private static final	String	LENGTH = "length";
	private static final	String	BUFFER = "buffer";
	
	int mNumber = 0;
	int mLength = 0;
	String mBuffer = "";
	
	public Packet() {
	}
	
	public Packet(int number, int length, String buffer) {
		mNumber = number;
		mLength = length;
		mBuffer = buffer;
	}
	
	public int getNumber() {
		return mNumber;
	}
	
	public int getLength() {
		return mLength;
	}
	
	public String getBuffer() {
		return mBuffer;
	}

	@Override
	public Object toJSON() throws JSONException {
		JSONObject json = new JSONObject();
		json.put(NUMBER, mNumber);
		json.put(LENGTH, mLength);
		json.put(BUFFER, mBuffer);
		
		return json;
	}

	@Override
	public void fromJSON(Object obj) throws JSONException {
		JSONObject json = (JSONObject) obj;
		mNumber = json.getInt(NUMBER);
		mLength = json.getInt(LENGTH);
		mBuffer = json.getString(BUFFER);
	}	
}

// End of Packet