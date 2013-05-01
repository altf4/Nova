package com.datasoft.ceres;

import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

public class ClassificationGridAdapter extends ArrayAdapter<String> {
  private final Context m_context;
  private List<String> m_values;
  private final Object m_lock = new Object();
  
  public ClassificationGridAdapter(Context ctx, List<String> vals) {
	  super(ctx, R.layout.activity_grid, vals);
	  m_context = ctx;
	  m_values = vals;
	  Collections.sort(m_values, new RowComparator());
  }
  
  @Override
  public void notifyDataSetChanged() {
	  Collections.sort(m_values, new RowComparator());
	  super.notifyDataSetChanged();
  }
  
  public void addAll(List<String> vals)
  {
	  synchronized(m_lock) {
		  m_values.addAll(vals);
	  }
  }
  
  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
	  LayoutInflater inflater = (LayoutInflater) m_context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
	  View rowView = inflater.inflate(R.layout.activity_grid, parent, false);
	  
	  TextView ip = (TextView)rowView.findViewById(R.id.suspectIpString);
	  TextView iface = (TextView)rowView.findViewById(R.id.suspectIfaceString);
	  TextProgressBar classification = (TextProgressBar)rowView.findViewById(R.id.suspectClass);
	  
	  String s = m_values.get(position);
	  String[] splitStr = s.split(":");
	  
	  ip.setText(splitStr[0]);
	  iface.setText(splitStr[1]);
	  classification.setText(splitStr[3]);
	  if(splitStr[2].equals("0"))
	  {
		  classification.setProgressDrawable(classification.getResources().getDrawable(R.drawable.greenbar));
	  }
	  int progress = 0;
	  double classVal = Double.parseDouble(splitStr[3].substring(0, splitStr[3].length() - 1));
	  progress = (int)Math.round(classVal);
	  classification.setProgress(progress);
	
	  return rowView;
  }
  
  private class RowComparator implements Comparator<String>
  {
	  @Override
	  public int compare(String left, String right)
	  {
		  String leftClass = left.split(":")[3];
		  String rightClass = right.split(":")[3];
		  String splitleft = leftClass.substring(0, leftClass.length() - 1);
		  String splitright = rightClass.substring(0, rightClass.length() - 1);
		  
		  Float check = Float.valueOf(splitright) - Float.valueOf(splitleft);
		  
		  if(check < 0)
		  {
			  return -1;
		  }
		  else if(check == 0)
		  {
			  return 0;
		  }
		  else
		  {
			  return 1;
		  }
	  }
  }
}
