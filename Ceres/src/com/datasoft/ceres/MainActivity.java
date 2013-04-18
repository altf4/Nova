package com.datasoft.ceres;

import android.os.AsyncTask;
import android.os.Bundle;
import android.app.Activity;
import android.app.ProgressDialog;
import android.view.Menu;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import org.json.*;

import java.net.URI;
import java.net.URISyntaxException;
import de.roderick.weberknecht.*;

public class MainActivity extends Activity {
	Button connect;
	EditText id;
	EditText ip;
	EditText passwd;
	EditText error;
	ProgressDialog dialog;
	CeresClient global;
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        global = new CeresClient();
        connect = (Button)findViewById(R.id.ceresConnect);
        id = (EditText)findViewById(R.id.credID);
        ip = (EditText)findViewById(R.id.credIP);
        passwd = (EditText)findViewById(R.id.credPW);
        error = (EditText)findViewById(R.id.error);
        dialog = new ProgressDialog(this);
        
        connect.setOnClickListener(new Button.OnClickListener() {
        	public void onClick(View v){
        		new CeresClientRequest().execute(ip.getText().toString(), "getAll", id.getText().toString());
        		error.setVisibility(0);
        	}
        });
    }


    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }
 
    private class CeresClientRequest extends AsyncTask<String, Void, Integer>
    {
    	@Override
    	protected void onPreExecute()
    	{
    		super.onPreExecute();
    		// Display spinner here
    		dialog.setCancelable(true);
    		dialog.setMessage("Attempting to connect");
    		dialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
    		dialog.show();
    	}
    	@Override
    	protected Integer doInBackground(String... params)
    	{
    		// Negotiate connection to Ceres.js websocket server
    		try
    		{
    			URI url = new URI("ws://" + params[0]);
    			global.ws = new WebSocketConnection(url);
    			global.ws.setEventHandler(new WebSocketEventHandler() {
					@Override
					public void onOpen() {
						// TODO Auto-generated method stub
					}
					@Override
					public void onMessage(WebSocketMessage message) {
						// TODO Auto-generated method stub
					}
					@Override
					public void onClose() {
						// TODO Auto-generated method stub
					}
				});
    			global.ws.connect();
    			JSONObject message = new JSONObject();
    			try
    			{
    				message.put("type", params[1]);
    				message.put("id", params[2]);
    			}
    			catch(JSONException jse)
    			{
    				jse.printStackTrace();
    				return 0;
    			}
    			global.ws.send(message.toString());
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
    			error.setVisibility(1);
    		}
    		else
    		{
    			dialog.dismiss();
    			// Move to new activity
    		}
    	}
    }
}