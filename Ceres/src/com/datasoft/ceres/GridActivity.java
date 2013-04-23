package com.datasoft.ceres;

import java.io.IOException;
import java.util.ArrayList;

import org.json.JSONException;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlPullParserFactory;

import de.roderick.weberknecht.WebSocketException;

import android.app.AlertDialog;
import android.app.ListActivity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.view.View;
import android.widget.ListView;
import android.widget.Toast;

public class GridActivity extends ListActivity {
	CeresClient global;
	ProgressDialog wait;
	ClassificationGridAdapter aa;
	Context gridContext;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        global = (CeresClient)getApplicationContext();
        wait = new ProgressDialog(this);
		gridContext = this;
		new ParseXml().execute();
	}
	
	@Override
	protected void onListItemClick(ListView l, View v, int position, long id) {
		String[] item = ((String)getListAdapter().getItem(position)).split(":");
		String selected = item[0] + ":" + item[1];
		Toast.makeText(this, selected + " selected", Toast.LENGTH_LONG).show();
		try
		{
			wait.setCancelable(true);
			wait.setMessage("Retrieving Suspect " + selected);
			wait.setProgressStyle(ProgressDialog.STYLE_SPINNER);
			wait.show();
			global.sendCeresRequest("getSuspect", "doop", selected);
		}
		catch(JSONException jse)
		{
			wait.cancel();
			Toast.makeText(this, "Could not get details for " + selected, Toast.LENGTH_LONG).show();
		}
		catch(WebSocketException wse)
		{
			wait.cancel();
			Toast.makeText(this, "Could not get details for " + selected, Toast.LENGTH_LONG).show();
		}
	}
	
	private class ParseXml extends AsyncTask<Void, Integer, ArrayList<String>> {
		@Override
		protected void onPreExecute()
		{
			wait.setCancelable(true);
			wait.setMessage("Retrieving Suspect List");
			wait.setProgressStyle(ProgressDialog.STYLE_SPINNER);
			wait.show();
    		super.onPreExecute();
		}
		
		@Override
		protected ArrayList<String> doInBackground(Void... vd)
		{
			try
			{
				XmlPullParserFactory factory = XmlPullParserFactory.newInstance();
				factory.setNamespaceAware(true);
				XmlPullParser xpp;
				int i = 0;
				// Ugly as hell, but I don't know what else to do. Just waits
				// 3 seconds for the XML to be taken from the wire; if it takes
				// longer, fail. Would rather wait until it comes, but there needs
				// to be a cut off at some point or it'll spin forever.
				while(!global.checkMessageReceived() && i < 5)
				{
					Thread.sleep(1000);
					i++;
				}
				if(global.checkMessageReceived())
				{
					xpp = factory.newPullParser();
					xpp.setInput(global.getXmlReceive());
					int evt = xpp.getEventType();
					ArrayList<String> al = new ArrayList<String>();
					// On this page, we're receiving a format containing three things:
					// ip, interface and classification
					String rowData = "";
					while(evt != XmlPullParser.END_DOCUMENT)
					{
						switch(evt)
						{
							case(XmlPullParser.START_DOCUMENT):
								break;
							case(XmlPullParser.START_TAG):
								if(xpp.getName().equals("suspect"))
								{
									rowData += xpp.getAttributeValue(null, "ipaddress") + ":";
									rowData += xpp.getAttributeValue(null, "interface") + ":";
								}
								break;
							case(XmlPullParser.TEXT): 
								rowData += xpp.getText() + "%";
								al.add(rowData);
								rowData = "";
								break;
							default: 
								break;
						}
						evt = xpp.next();
					}
					global.clearXmlReceive();
					return al;
				}
				else
				{
					return null;
				}
			}
			catch(XmlPullParserException xppe)
			{
				return null;
			}
			catch(IOException ioe)
			{
				return null;
			}
			catch(InterruptedException ie)
			{
				return null;
			}
		}
		
		@Override
		protected void onPostExecute(ArrayList<String> gridPop)
		{
			if(gridPop == null || gridPop.size() == 0)
			{
				wait.cancel();
				AlertDialog.Builder build = new AlertDialog.Builder(gridContext);
				build
				.setTitle("Suspect list empty")
				.setMessage("Ceres server returned no suspects. Try again?")
				.setCancelable(false)
				.setPositiveButton("Yes", new DialogInterface.OnClickListener(){
					@Override
					public void onClick(DialogInterface dialog, int id)
					{
		    			Intent nextPage = new Intent(getApplicationContext(), GridActivity.class);
		    			nextPage.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
		    			nextPage.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		    			getApplicationContext().startActivity(nextPage);
					}
				})
				.setNegativeButton("No", new DialogInterface.OnClickListener() {
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
				ad.show();
			}
			else
			{
				Toast.makeText(gridContext, gridPop.size() + " suspects loaded", Toast.LENGTH_LONG).show();
				aa = new ClassificationGridAdapter(gridContext, gridPop);
		        setListAdapter(aa);
				wait.cancel();
			}
		}
	}
}
