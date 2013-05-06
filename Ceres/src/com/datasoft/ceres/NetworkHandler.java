package com.datasoft.ceres;

import com.loopj.android.http.*;

public class NetworkHandler {
	private static AsyncHttpClient m_client = new AsyncHttpClient();
	
	public static void get(String url, RequestParams params, AsyncHttpResponseHandler response)
	{
		m_client.get(url,  params, response);
	}
	
	public static void post(String url, RequestParams params, AsyncHttpResponseHandler response)
	{
		m_client.post(url,  params, response);
	}
}
