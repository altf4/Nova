package com.datasoft.ceres;

import java.io.StringReader;
import java.net.URI;
import java.net.URISyntaxException;

import org.json.JSONException;
import org.json.JSONObject;

import android.app.Application;
import android.content.Intent;
import de.roderick.weberknecht.*;

public class CeresClient extends Application {
	WebSocket m_ws;
	String m_xmlBase;
	StringReader m_xmlReceive;
	Boolean m_messageReceived;
	
	@Override
	public void onCreate()
	{
		m_messageReceived = false;
		super.onCreate();
	}
	
	public void initWebSocketConnection(String loc) throws WebSocketException, URISyntaxException
	{
		URI url = new URI("ws://" + loc + "/");
		m_ws = new WebSocketConnection(url);
		m_ws.setEventHandler(new WebSocketEventHandler() {
			@Override
			public void onOpen() {
				// TODO Auto-generated method stub
			}
			@Override
			public void onMessage(WebSocketMessage message) {
				String parse = message.getText();
				m_xmlBase = parse;
				m_messageReceived = true;
			}
			@Override
			public void onClose() {
				System.out.println("Lost connection!");
				Intent intent = new Intent(getApplicationContext(), MainActivity.class);
				intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
				intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
				getApplicationContext().startActivity(intent);
				// TODO: Need to find a way to pop a dialog to tell the user they've 
				// lost connection and need to reconnect. Right now, if the server quits
				// on you while connected, it just shoves you back to the login screen
				// (more of a measure to see how the intent stuff works at present)
			}
		});
		m_ws.connect();
	}

	public void sendCeresRequest(String type, String id) throws WebSocketException, JSONException
	{
		JSONObject message = new JSONObject();
		message.put("type", type);
		message.put("id", id);
		m_ws.send(message.toString());
	}
	
	public void sendCeresRequest(String type, String id, String suspect) throws WebSocketException, JSONException
	{
		JSONObject message = new JSONObject();
		message.put("type", type);
		message.put("id", id);
		if(suspect != null && !suspect.equals(""))
		{
			message.put("suspect", suspect);
		}
		m_ws.send(message.toString());
	}
	
	public Boolean checkMessageReceived()
	{
		return m_messageReceived;
	}
	
	public StringReader getXmlReceive()
	{
		m_xmlReceive = new StringReader(m_xmlBase);
		return m_xmlReceive;
	}
	
	public void clearXmlReceive()
	{
		m_xmlReceive = null;
		m_messageReceived = false;
	}
}
