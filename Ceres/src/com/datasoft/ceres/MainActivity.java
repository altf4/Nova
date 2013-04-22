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
	Button connect;
	EditText id;
	EditText ip;
	EditText passwd;
	EditText notify;
	EditText success;
	ProgressDialog dialog;
	CeresClient global;
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        global = (CeresClient)getApplicationContext();
        connect = (Button)findViewById(R.id.ceresConnect);
        id = (EditText)findViewById(R.id.credID);
        ip = (EditText)findViewById(R.id.credIP);
        passwd = (EditText)findViewById(R.id.credPW);
        notify = (EditText)findViewById(R.id.notify);
        dialog = new ProgressDialog(this);
        connect.setOnClickListener(new Button.OnClickListener() {
        	public void onClick(View v){
        		new CeresClientConnect().execute(ip.getText().toString(), "getAll", id.getText().toString());
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
    			global.ws.close();
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
    		dialog.setCancelable(true);
    		dialog.setMessage("Attempting to connect");
    		dialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
    		dialog.show();
    		super.onPreExecute();
    	}
    	@Override
    	protected Integer doInBackground(String... params)
    	{
    		// Negotiate connection to Ceres.js websocket server
    		try
    		{
    			global.initWebSocketConnection(params[0]);
    			global.sendCeresRequest(params[1], params[2]);
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
    			dialog.dismiss();
    			try
    			{
    				global.ws.close();	
    			}
    			catch(WebSocketException wse)
    			{
    				wse.printStackTrace();
    			}
    			// error
    			notify.setText(R.string.error);
    			notify.setTextColor(Color.RED);
    			notify.setVisibility(1);
    		}
    		else
    		{
    			dialog.dismiss();
    			//success
    			notify.setText(R.string.success);
    			notify.setTextColor(Color.GREEN);
    			notify.setVisibility(1);
    			// wait a second, move to next activity
    			Intent nextPage = new Intent(getApplicationContext(), GridActivity.class);
    			nextPage.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    			getApplicationContext().startActivity(nextPage);
    		}
    	}
    }
}