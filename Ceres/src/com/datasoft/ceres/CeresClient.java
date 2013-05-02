package com.datasoft.ceres;

import java.io.StringReader;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.ArrayList;

import org.json.JSONException;
import org.json.JSONObject;

import android.app.AlertDialog;
import android.app.Application;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.TaskStackBuilder;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.widget.Toast;
import de.roderick.weberknecht.*;

public class CeresClient extends Application {
	WebSocket m_ws;
	String m_clientId;
	String m_xmlBase;
	StringReader m_xmlReceive;
	Boolean m_messageReceived;
	ArrayList<String> m_gridCache;
	Boolean m_isInForeground;
	
	@Override
	public void onCreate()
	{
		m_messageReceived = false;
		m_clientId = "";
		m_gridCache = new ArrayList<String>();
		super.onCreate();
	}
	
	public ArrayList<String> getGridCache()
	{
		return m_gridCache;
	}
	
	public void setGridCache(ArrayList<String> newCache)
	{
		m_gridCache.clear();
		for(String s : newCache)
		{
			m_gridCache.add(s);
		}
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
				if(isInForeground())
				{
					System.out.println("Connection lost!");
					/*AlertDialog.Builder build = new AlertDialog.Builder(getApplicationContext());
					build
					.setTitle("Connection Lost")
					.setMessage("Ceres has lost connection with the Server. Stay on page?")
					.setCancelable(false)
					.setPositiveButton("Yes", new DialogInterface.OnClickListener(){
						@Override
						public void onClick(DialogInterface dialog, int id)
						{
							dialog.cancel();
						}
					})
					.setNegativeButton("No, go to login", new DialogInterface.OnClickListener() {
						@Override
						public void onClick(DialogInterface dialog, int which)
						{
			    			Intent nextPage = new Intent(getApplicationContext(), MainActivity.class);
			    			nextPage.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
			    			nextPage.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
			    			getApplicationContext().startActivity(nextPage);
						}
					});
					AlertDialog ad = build.create();
					ad.show();*/
					
					// TODO: For now, it seems like there's now way to call the UI Thread from 
					// the Application level; no access to the current Activity, and any call
					// to something that should be fired on the UI Thread will cause an exception
					// to be thrown. Until I can find a tricky way to have a dialog activated here,
					// both inForeground and !inForeground will trigger an alert.
					
					Context ctx = getApplicationContext();
					Notification.Builder build = new Notification.Builder(ctx)
						.setSmallIcon(R.drawable.ic_launcher)
						.setContentTitle("Ceres Lost Connection")
						.setContentText("The Ceres app has lost connection with the server")
						.setAutoCancel(true);
					Intent intent = new Intent(ctx, MainActivity.class);
					TaskStackBuilder tsbuild = TaskStackBuilder.create(ctx);
					tsbuild.addParentStack(MainActivity.class);
					tsbuild.addNextIntent(intent);
					PendingIntent rpintent = tsbuild.getPendingIntent(0, PendingIntent.FLAG_UPDATE_CURRENT);
					build.setContentIntent(rpintent);
					NotificationManager nm = (NotificationManager)getSystemService(Context.NOTIFICATION_SERVICE);
					nm.notify(0, build.build());
				}
				else
				{
					Context ctx = getApplicationContext();
					Notification.Builder build = new Notification.Builder(ctx)
						.setSmallIcon(R.drawable.ic_launcher)
						.setContentTitle("Ceres Lost Connection")
						.setContentText("The Ceres app has lost connection with the server")
						.setAutoCancel(true);
					Intent intent = new Intent(ctx, MainActivity.class);
					TaskStackBuilder tsbuild = TaskStackBuilder.create(ctx);
					tsbuild.addParentStack(MainActivity.class);
					tsbuild.addNextIntent(intent);
					PendingIntent rpintent = tsbuild.getPendingIntent(0, PendingIntent.FLAG_UPDATE_CURRENT);
					build.setContentIntent(rpintent);
					NotificationManager nm = (NotificationManager)getSystemService(Context.NOTIFICATION_SERVICE);
					nm.notify(0, build.build());
				}
			}
		});
		m_ws.connect();
	}

	public void sendCeresRequest(String type, String id) throws WebSocketException, JSONException
	{
		JSONObject message = new JSONObject();
		message.put("type", type);
		message.put("id", id);
		if(m_clientId == "")
		{
			m_clientId = id;
		}
		m_ws.send(message.toString());
	}
	
	public void sendCeresRequest(String type, String id, String suspect) throws WebSocketException, JSONException
	{
		JSONObject message = new JSONObject();
		message.put("type", type);
		message.put("id", id);
		if(m_clientId == "")
		{
			m_clientId = id;
		}
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
	
	public String getClientId()
	{
		return m_clientId;
	}
	
	public void setClientId(String id)
	{
		m_clientId = id;
	}
	
	public Boolean isInForeground()
	{
		return m_isInForeground;
	}
	
	public void setForeground(Boolean inForeground)
	{
		m_isInForeground = inForeground;
	}
}
