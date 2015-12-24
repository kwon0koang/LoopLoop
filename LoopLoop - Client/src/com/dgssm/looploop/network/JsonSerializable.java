package com.dgssm.looploop.network;

import org.json.JSONException;

public interface JsonSerializable {
	public Object toJSON() throws JSONException;
    public void fromJSON(Object obj) throws JSONException;
}
