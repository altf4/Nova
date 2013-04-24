package com.datasoft.ceres;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.widget.TextView;

public class DetailsActivity extends Activity
{
	TextView m_textView;
	
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
	    super.onCreate(savedInstanceState);
	    m_textView = new TextView(this);
	    m_textView.setTextSize(20);
	    Intent intent = getIntent();
	    String suspectID = intent.getStringExtra(GridActivity.DETAILS_MESSAGE);
	    m_textView.setText(suspectID);
	    setContentView(m_textView);
	}

}
