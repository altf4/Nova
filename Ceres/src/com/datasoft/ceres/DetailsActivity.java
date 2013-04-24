package com.datasoft.ceres;

import java.io.IOException;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlPullParserFactory;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.widget.TextView;
import android.widget.Toast;

public class DetailsActivity extends Activity
{
	TextView m_id;
	TextView m_ip;
	TextView m_iface;
	TextView m_class;
	TextView m_lpt;
	TextView m_tcp;
	TextView m_udp;
	TextView m_icmp;
	TextView m_other;
	TextView m_rst;
	TextView m_syn;
	TextView m_ack;
	TextView m_fin;
	TextView m_synack;
	ProgressDialog m_wait;
	Context m_detailsContext;
	CeresClient m_global;
	
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
	    super.onCreate(savedInstanceState);
	    setContentView(R.layout.activity_details);
	    m_global = (CeresClient)getApplicationContext();
	    m_wait = new ProgressDialog(this);
	    m_id = (TextView)findViewById(R.id.suspectId);
	    m_ip = (TextView)findViewById(R.id.suspectIpString);
	    m_iface = (TextView)findViewById(R.id.suspectIfaceString);
	    m_class = (TextView)findViewById(R.id.suspectClassification);
	    m_lpt = (TextView)findViewById(R.id.suspectLastPacket);
	    m_tcp = (TextView)findViewById(R.id.suspectTcpCount);
	    m_udp = (TextView)findViewById(R.id.suspectUdpCount);
	    m_icmp = (TextView)findViewById(R.id.suspectIcmpCount);
	    m_other = (TextView)findViewById(R.id.suspectOtherCount);
	    m_rst = (TextView)findViewById(R.id.suspectRstCount);
	    m_syn = (TextView)findViewById(R.id.suspectSynCount);
	    m_ack = (TextView)findViewById(R.id.suspectAckCount);
	    m_fin = (TextView)findViewById(R.id.suspectFinCount);
	    m_synack = (TextView)findViewById(R.id.suspectSynAckCount);
	    m_detailsContext = this;
	    new ParseXml().execute();
	}
	
	private class ParseXml extends AsyncTask<Void, Void, Suspect> {
		@Override
		protected void onPreExecute()
		{
			m_wait.setCancelable(true);
			m_wait.setMessage("Retrieving Suspect Data");
			m_wait.setProgressStyle(ProgressDialog.STYLE_SPINNER);
			m_wait.show();
    		super.onPreExecute();
		}
		
		@Override
		protected Suspect doInBackground(Void... vd)
		{
			try
			{
				XmlPullParserFactory factory = XmlPullParserFactory.newInstance();
				factory.setNamespaceAware(true);
				XmlPullParser xpp;

				if(m_global.checkMessageReceived())
				{
					xpp = factory.newPullParser();
					xpp.setInput(m_global.getXmlReceive());
					Suspect parsed = new Suspect();
					int evt = xpp.getEventType();
					// On this page, we're receiving a format containing three things:
					// ip, interface and classification
					while(evt != XmlPullParser.END_DOCUMENT)
					{
						switch(evt)
						{
							case(XmlPullParser.START_TAG):
								if(xpp.getName().equals("idInfo"))
								{
									parsed.m_id = xpp.getAttributeValue(null, "id");
									parsed.m_ip = xpp.getAttributeValue(null, "ip");
									parsed.m_iface = xpp.getAttributeValue(null, "iface");
									parsed.m_classification = xpp.getAttributeValue(null, "class");
									parsed.m_lastPacket = xpp.getAttributeValue(null, "lpt");
								}
								else if(xpp.getName().equals("protoInfo"))
								{
									parsed.m_tcpCount = xpp.getAttributeValue(null, "tcp");
									parsed.m_udpCount = xpp.getAttributeValue(null, "udp");
									parsed.m_icmpCount = xpp.getAttributeValue(null, "icmp");
									parsed.m_otherCount = xpp.getAttributeValue(null, "other");
								}
								else if(xpp.getName().equals("packetInfo"))
								{
									parsed.m_rstCount = xpp.getAttributeValue(null, "rst");
									parsed.m_ackCount = xpp.getAttributeValue(null, "ack");
									parsed.m_synCount = xpp.getAttributeValue(null, "syn");
									parsed.m_finCount = xpp.getAttributeValue(null, "fin");
									parsed.m_synAckCount = xpp.getAttributeValue(null, "synack");
								}
								break;
							default: 
								break;
						}
						evt = xpp.next();
					}
					return parsed;
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
		protected void onPostExecute(Suspect suspect)
		{
			if(suspect == null)
			{
				m_wait.cancel();
				AlertDialog.Builder build = new AlertDialog.Builder(m_detailsContext);
				build
				.setTitle("Suspect list empty")
				.setMessage("Ceres server returned no suspect data. Try again?")
				.setCancelable(false)
				.setPositiveButton("Yes", new DialogInterface.OnClickListener(){
					@Override
					public void onClick(DialogInterface dialog, int id)
					{
						m_global.clearXmlReceive();
		    			Intent nextPage = new Intent(getApplicationContext(), DetailsActivity.class);
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
				m_id.setText(suspect.m_id);
				m_ip.setText(suspect.m_ip);
				m_iface.setText(suspect.m_iface);
				m_class.setText(suspect.m_classification);
				m_lpt.setText(suspect.m_lastPacket);
				m_tcp.setText(suspect.m_tcpCount);
				m_udp.setText(suspect.m_udpCount);
				m_icmp.setText(suspect.m_icmpCount);
				m_other.setText(suspect.m_otherCount);
				m_rst.setText(suspect.m_rstCount);
				m_syn.setText(suspect.m_synCount);
				m_ack.setText(suspect.m_ackCount);
				m_fin.setText(suspect.m_finCount);
				m_synack.setText(suspect.m_synAckCount);
				Toast.makeText(m_detailsContext, "Suspect " + suspect.getIp() + ":" + suspect.getIface() + " loaded", Toast.LENGTH_LONG).show();
				m_wait.cancel();
			}
		}
	}
}
