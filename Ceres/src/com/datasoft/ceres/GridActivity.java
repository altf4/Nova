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
	CeresClient m_global;
	ProgressDialog m_wait;
	ClassificationGridAdapter m_aa;
	Context m_gridContext;
	String m_selected;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        m_global = (CeresClient)getApplicationContext();
        m_wait = new ProgressDialog(this);
		m_gridContext = this;
		new ParseXml().execute();
	}
	
	@Override
	protected void onListItemClick(ListView l, View v, int position, long id) {
		String[] item = ((String)getListAdapter().getItem(position)).split(":");
		m_selected = item[0] + ":" + item[1];
		Toast.makeText(this, m_selected + " selected", Toast.LENGTH_SHORT).show();
	    new CeresSuspectRequest().execute();
	}
	
	private class CeresSuspectRequest extends AsyncTask<Void, Void, Integer> {
		@Override
		protected void onPreExecute()
		{
			m_global.clearXmlReceive();
			m_wait.setCancelable(true);
			m_wait.setMessage("Retrieving details for " + m_selected);
			m_wait.setProgressStyle(ProgressDialog.STYLE_SPINNER);
			m_wait.show();
			super.onPreExecute();
		}
		@Override
		protected Integer doInBackground(Void... vd)
		{
			try
			{
				m_global.sendCeresRequest("getSuspect", "doop", m_selected);
				while(!m_global.checkMessageReceived()){};
			}
			catch(JSONException jse)
			{
				return 0;
			}
			catch(WebSocketException wse)
			{
				return 0;
			}
			return 1;
		}
		@Override
		protected void onPostExecute(Integer result)
		{
			if(result == 0)
			{
				m_wait.cancel();
				Toast.makeText(m_gridContext, "Could not get details for " + m_selected, Toast.LENGTH_LONG).show();
			}
			else
			{
				m_wait.cancel();
				Toast.makeText(m_gridContext, "Would switch activity", Toast.LENGTH_LONG).show();
				/*Intent nextPage = new Intent(getApplicationContext(), DetailsActivity.class);
				nextPage.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
				nextPage.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
				getApplicationContext().startActivity(nextPage);*/
			}
		}
	}
	
	private class ParseXml extends AsyncTask<Void, Integer, ArrayList<String>> {
		@Override
		protected void onPreExecute()
		{
			m_wait.setCancelable(true);
			m_wait.setMessage("Retrieving Suspect List");
			m_wait.setProgressStyle(ProgressDialog.STYLE_SPINNER);
			m_wait.show();
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
				// int i = 0;
				// Ugly as hell, but I don't know what else to do. Just waits
				// 3 seconds for the XML to be taken from the wire; if it takes
				// longer, fail. Would rather wait until it comes, but there needs
				// to be a cut off at some point or it'll spin forever.
				//
				// Moved something similar to this into right after the 
				// message send in MainActivity to test responsiveness.
				// Works fine, will keep it there for now.
				/*while(!m_global.checkMessageReceived() && i < 5)
				{
					Thread.sleep(1000);
					i++;
				}*/
				if(m_global.checkMessageReceived())
				{
					xpp = factory.newPullParser();
					xpp.setInput(m_global.getXmlReceive());
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
		}
		
		@Override
		protected void onPostExecute(ArrayList<String> gridPop)
		{
			if(gridPop == null || gridPop.size() == 0)
			{
				m_wait.cancel();
				AlertDialog.Builder build = new AlertDialog.Builder(m_gridContext);
				build
				.setTitle("Suspect list empty")
				.setMessage("Ceres server returned no suspects. Try again?")
				.setCancelable(false)
				.setPositiveButton("Yes", new DialogInterface.OnClickListener(){
					@Override
					public void onClick(DialogInterface dialog, int id)
					{
						m_global.clearXmlReceive();
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
						m_global.clearXmlReceive();
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
				Toast.makeText(m_gridContext, gridPop.size() + " suspects loaded", Toast.LENGTH_LONG).show();
				m_aa = new ClassificationGridAdapter(m_gridContext, gridPop);
		        setListAdapter(m_aa);
				m_wait.cancel();
			}
		}
	}
}
