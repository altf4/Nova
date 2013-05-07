package com.datasoft.ceres;

import java.io.IOException;
import java.security.KeyManagementException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.UnrecoverableKeyException;
import java.security.cert.CertificateException;
import org.apache.http.conn.ssl.SSLSocketFactory;
import org.apache.http.conn.ssl.X509HostnameVerifier;
import android.content.Context;
import android.content.res.Resources.NotFoundException;
import com.loopj.android.http.*;

public class NetworkHandler {
	private static AsyncHttpClient m_client = new AsyncHttpClient();
	
	public static void setSSL(Context ctx, int keyid, char[] keystorePass)
	{
		try
		{
			KeyStore keystore = KeyStore.getInstance("BKS");
			keystore.load(ctx.getResources().openRawResource(keyid), keystorePass);
			SSLSocketFactory sslsf = new SSLSocketFactory(keystore);
			sslsf.setHostnameVerifier((X509HostnameVerifier)SSLSocketFactory.ALLOW_ALL_HOSTNAME_VERIFIER);
			m_client.setSSLSocketFactory(sslsf);
		}
		catch(KeyStoreException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		catch(CertificateException e) 
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		catch(NoSuchAlgorithmException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		catch(NotFoundException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		catch (KeyManagementException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		catch(IOException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		catch (UnrecoverableKeyException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	public static void get(String url, RequestParams params, AsyncHttpResponseHandler response)
	{
		m_client.get(url,  params, response);
	}
	
	public static void post(String url, RequestParams params, AsyncHttpResponseHandler response)
	{
		m_client.post(url,  params, response);
	}
}
