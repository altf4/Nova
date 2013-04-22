package com.datasoft.ceres;

import java.io.IOException;
import java.io.StringReader;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Vector;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlPullParserFactory;
import android.app.Activity;
import android.app.ListActivity;
import android.app.ProgressDialog;
import android.content.Context;
import android.os.AsyncTask;
import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
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
		String item = (String)getListAdapter().getItem(position);
		// Just to test that the clickables work, onListItemClick will switch intents
		// and query for suspect data
		Toast.makeText(this, item + " selected", Toast.LENGTH_LONG).show();
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
				while(!global.checkMessageReceived() && i < 3)
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
								rowData += xpp.getAttributeValue(null, "ipaddress") + ":";
								rowData += xpp.getAttributeValue(null, "interface") + ":";
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
			if(gridPop == null)
			{
				// TODO: Show problem dialog
			}
			else
			{
				aa = new ClassificationGridAdapter(gridContext, gridPop);
		        setListAdapter(aa);
			}
			wait.cancel();
		}
	}
}
