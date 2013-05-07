package com.datasoft.ceres;

import android.os.AsyncTask;
import android.os.Bundle;
import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.view.Menu;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import org.json.*;
import com.loopj.android.http.AsyncHttpResponseHandler;
import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStreamWriter;

public class MainActivity extends Activity {
	Context m_ctx;
	Button m_connect;
	EditText m_id;
	EditText m_ip;
	EditText m_port;
	EditText m_passwd;
	EditText m_notify;
	EditText m_success;
	CheckBox m_keepLogged;
	ProgressDialog m_dialog;
	CeresClient m_global;
	Boolean m_keepMeLoggedIn;
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        m_ctx = this;
        m_global = (CeresClient)getApplicationContext();
        m_global.setForeground(true);
        m_id = (EditText)findViewById(R.id.credID);
        m_ip = (EditText)findViewById(R.id.credIP);
        m_port = (EditText)findViewById(R.id.credPort);
        m_passwd = (EditText)findViewById(R.id.credPW);
        m_keepMeLoggedIn = false;
        
    	InputStream in = null;
        try
        {
        	File networkTarget = new File(getFilesDir(), "networkTarget");
        	if(networkTarget.exists() && networkTarget.length() != 0)
        	{
        		m_keepMeLoggedIn = true;
        		in = new BufferedInputStream(new FileInputStream(networkTarget));
        		byte[] buf = new byte[512];
        		if(in.read(buf, 0, 512) != -1)
        		{
        			String json = new String(buf);
        			JSONObject net = new JSONObject(json);
        			if(net.has("ip") && net.has("id") && net.has("port"))
        			{
        				m_ip.setText(net.get("ip").toString());
        				m_port.setText(net.get("port").toString());
        				m_id.setText(net.get("id").toString());
        			}
        		}
        		else
        		{
                	if(in != null)
                	{
                		in.close();
                	}
        		}
        	}
        }
        catch(IOException ioe)
        {
        }
        catch(JSONException jse)
        {
        }
        
        m_keepLogged = (CheckBox)findViewById(R.id.keepLogged);
        m_keepLogged.setChecked(m_keepMeLoggedIn);
        m_connect = (Button)findViewById(R.id.ceresConnect);
        m_id = (EditText)findViewById(R.id.credID);
        m_ip = (EditText)findViewById(R.id.credIP);
        m_passwd = (EditText)findViewById(R.id.credPW);
        m_notify = (EditText)findViewById(R.id.notify);
        m_dialog = new ProgressDialog(this);
        m_connect.setOnClickListener(new Button.OnClickListener() {
        	public void onClick(View v){
        		m_keepMeLoggedIn = ((CheckBox)findViewById(R.id.keepLogged)).isChecked();
        		if(m_keepMeLoggedIn)
        		{
        			try
        			{
        				File file = new File(getFilesDir(), "networkTarget");
        				OutputStreamWriter bos = new OutputStreamWriter(new FileOutputStream(file));
        				String writeToFile;
        				JSONObject json = new JSONObject();
        				json.put("ip", m_ip.getText().toString());
        				json.put("port", m_port.getText().toString());
        				json.put("id", m_id.getText().toString());
        				writeToFile = json.toString();
        				bos.write(writeToFile);
        				bos.flush();
        				bos.close();
        			}
        			catch(IOException ioe)
        			{
        			}
        			catch(JSONException jse)
        			{
        			}
        		}
        		else
        		{
    				File file = new File(getFilesDir(), "networkTarget");
    				file.delete();
        		}
        		String netString = (m_ip.getText().toString() + ":" + m_port.getText().toString());
        		new CeresClientConnect().execute(netString);
        		m_notify.setVisibility(View.INVISIBLE);
        	}
        });
    }
    
    public void onCheckboxClicked(View view)
    {
    	m_keepMeLoggedIn = ((CheckBox) view).isChecked();
    }
    
    @Override
    protected void onPause()
    {
    	super.onPause();
    	m_global.setForeground(false);
    }
    
    @Override
    protected void onResume()
    {
    	super.onResume();
    	m_global.setForeground(true);
    }
    
    @Override
    protected void onRestart()
    {
    	InputStream in = null;
        try
        {
        	File networkTarget = new File(getFilesDir(), "networkTarget");
        	if(networkTarget.exists() && networkTarget.length() != 0)
        	{
        		in = new BufferedInputStream(new FileInputStream(networkTarget));
        		byte[] buf = new byte[512];
        		if(in.read(buf, 0, 512) != -1)
        		{
        			String json = new String(buf);
        			JSONObject net = new JSONObject(json);
        			if(net.has("ip") && net.has("id") && net.has("port"))
        			{
        				m_ip.setText(net.get("ip").toString());
        				m_port.setText(net.get("port").toString());
        				m_id.setText(net.get("id").toString());
        			}
        		}
        		else
        		{
                	if(in != null)
                	{
                		in.close();
                	}
        		}
        	}
        }
        catch(IOException ioe)
        {
        }
        catch(JSONException jse)
        {
        }
        super.onRestart();
    }
    
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }
 
    private class CeresClientConnect extends AsyncTask<String, Void, Integer> {
    	@Override
    	protected void onPreExecute()
    	{
    		// Display spinner here
    		if(m_dialog == null)
    		{
    			m_dialog = new ProgressDialog(m_ctx);
    		}
    		m_dialog.setCancelable(true);
    		m_dialog.setMessage("Attempting to connect");
    		m_dialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
    		m_dialog.show();
    		super.onPreExecute();
    	}
    	
    	@Override
    	protected Integer doInBackground(String... params)
    	{
    		// Negotiate connection to Ceres.js websocket server
    		// params[0] == url
    		try
    		{
    			m_global.setURL(params[0]);
    			NetworkHandler.setSSL(m_ctx, R.raw.test, m_passwd.getText().toString().toCharArray());
    			NetworkHandler.get(("https://" + m_global.getURL() + "/getAll"), null, new AsyncHttpResponseHandler(){
    				@Override
    				public void onSuccess(String xml)
    				{
    					m_global.setXmlBase(xml);
    					m_global.setMessageReceived(true);
    				}
    				
    				@Override
    				public void onFailure(Throwable err, String content)
    				{
    					m_global.setXmlBase("");
    					m_global.setMessageReceived(true);
    				}
    			});
    			while(!m_global.checkMessageReceived()){};
    		}
    		catch(Exception e)
    		{
    			e.printStackTrace();
    			return 0;
    		}
    		
    		if(m_global.getXmlBase() != "")
    		{
    			System.out.println("returning 1");
    			return 1;
    		}
    		else
    		{
    			System.out.println("returning 0");
    			return 0;
    		}
    	}
    	@Override
    	protected void onPostExecute(Integer result)
    	{
    		// After doInBackground is done the return value for it
    		// is sent to this method
    		if(result == 0)
    		{
    			// Display dialog saying connection failed,
    			// redirect back to login page
    			m_dialog.dismiss();
    			m_notify.setText(R.string.error);
    			m_notify.setTextColor(Color.RED);
    			m_notify.setVisibility(View.VISIBLE);
    		}
    		else
    		{
    			m_dialog.dismiss();
    			m_notify.setText(R.string.success);
    			m_notify.setTextColor(Color.GREEN);
    			m_notify.setVisibility(View.VISIBLE);
    			Intent nextPage = new Intent(getApplicationContext(), GridActivity.class);
    			nextPage.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    			nextPage.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
    			getApplicationContext().startActivity(nextPage);
    		}
    	}
    }
}