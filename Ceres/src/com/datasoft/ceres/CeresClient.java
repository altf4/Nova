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
	WebSocket ws;
	StringReader xmlReceive;
	Boolean messageReceived;
	
	@Override
	public void onCreate()
	{
		System.out.println("In CeresClient onCreate()");
		messageReceived = false;
		super.onCreate();
	}
	
	public void initWebSocketConnection(String loc) throws WebSocketException, URISyntaxException
	{
		URI url = new URI("ws://" + loc + "/");
		ws = new WebSocketConnection(url);
		ws.setEventHandler(new WebSocketEventHandler() {
			@Override
			public void onOpen() {
				// TODO Auto-generated method stub
			}
			@Override
			public void onMessage(WebSocketMessage message) {
				String parse = message.getText();
				System.out.println("parse == " + parse);
				xmlReceive = new StringReader(parse);
				messageReceived = true;
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
		ws.connect();
	}
	
	public void sendCeresRequest(String type, String id) throws WebSocketException, JSONException
	{
		JSONObject message = new JSONObject();
		message.put("type", type);
		message.put("id", id);
		ws.send(message.toString());
	}
	
	public Boolean checkMessageReceived()
	{
		return messageReceived;
	}
	
	public StringReader getXmlReceive()
	{
		return xmlReceive;
	}
	
	public void clearXmlReceive()
	{
		xmlReceive = null;
		messageReceived = false;
	}
}
