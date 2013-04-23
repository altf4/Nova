package com.datasoft.ceres;

import java.util.List;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

public class ClassificationGridAdapter extends ArrayAdapter<String> {
  private final Context context;
  private final List<String> values;
  
  public ClassificationGridAdapter(Context ctx, List<String> vals) {
	  super(ctx, R.layout.activity_grid, vals);
	  context = ctx;
	  values = vals;
  }
  
  @Override
  public void notifyDataSetChanged() {
	  
	  super.notifyDataSetChanged();
  }
  
  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
	  LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
	  View rowView = inflater.inflate(R.layout.activity_grid, parent, false);
	  TextView ip = (TextView) rowView.findViewById(R.id.suspectIp);
	  TextView iface = (TextView) rowView.findViewById(R.id.suspectIface);
	  TextView classification = (TextView) rowView.findViewById(R.id.suspectClass);
	  String s = values.get(position);
	  String[] splitStr = s.split(":");
	  ip.setText(splitStr[0]);
	  iface.setText(splitStr[1]);
	  classification.setText(splitStr[2]);
	
	  return rowView;
  }
}
