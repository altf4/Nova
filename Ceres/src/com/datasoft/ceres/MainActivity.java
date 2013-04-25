package com.datasoft.ceres;

import android.os.AsyncTask;
import android.os.Bundle;
import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Intent;
import android.graphics.Color;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import org.json.*;
import java.net.URISyntaxException;
import de.roderick.weberknecht.*;

public class MainActivity extends Activity {
	Button m_connect;
	EditText m_id;
	EditText m_ip;
	EditText m_passwd;
	EditText m_notify;
	EditText m_success;
	ProgressDialog m_dialog;
	CeresClient m_global;
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        m_global = (CeresClient)getApplicationContext();
        m_connect = (Button)findViewById(R.id.ceresConnect);
        m_id = (EditText)findViewById(R.id.credID);
        m_ip = (EditText)findViewById(R.id.credIP);
        m_passwd = (EditText)findViewById(R.id.credPW);
        m_notify = (EditText)findViewById(R.id.notify);
        m_dialog = new ProgressDialog(this);
        m_connect.setOnClickListener(new Button.OnClickListener() {
        	public void onClick(View v){
        		new CeresClientConnect().execute(m_ip.getText().toString(), "getAll", m_id.getText().toString());
        		m_notify.setVisibility(View.INVISIBLE);
        	}
        });
    }
    
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent keyEvent)
    {
    	if(keyCode == KeyEvent.KEYCODE_HOME)
    	{
    		// If home key is hit, close connection.
    		try
    		{
    			m_global.m_ws.close();
    		}
    		catch(WebSocketException wse)
    		{
    			System.out.println("Could not close connection!");
    		}
    	}
    	return false;
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
    		try
    		{
    			m_global.initWebSocketConnection(params[0]);
    			m_global.sendCeresRequest(params[1], params[2]);
    			while(!m_global.checkMessageReceived()){};
    		}
    		catch(WebSocketException wse)
    		{
    			wse.printStackTrace();
    			return 0;
    		}
    		catch(URISyntaxException use)
    		{
    			use.printStackTrace();
    			return 0;
    		}
			catch(JSONException jse)
			{
				jse.printStackTrace();
				return 0;
			}
    		catch(Exception e)
    		{
    			e.printStackTrace();
    			return 0;
    		}
    		return 1;
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
    			try
    			{
    				m_global.m_ws.close();	
    			}
    			catch(WebSocketException wse)
    			{
    				wse.printStackTrace();
    			}
    			// error
    			m_notify.setText(R.string.error);
    			m_notify.setTextColor(Color.RED);
    			m_notify.setVisibility(View.VISIBLE);
    		}
    		else
    		{
    			m_dialog.dismiss();
    			//success
    			m_notify.setText(R.string.success);
    			m_notify.setTextColor(Color.GREEN);
    			m_notify.setVisibility(View.VISIBLE);
    			// wait a second, move to next activity
    			Intent nextPage = new Intent(getApplicationContext(), GridActivity.class);
    			nextPage.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    			getApplicationContext().startActivity(nextPage);
    		}
    	}
    }
}