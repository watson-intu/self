
package com.self_instance_android;

import android.app.Activity;
import android.widget.TextView;
import android.os.Bundle;

public class SelfMain extends Activity {
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		TextView tv = new TextView(this);
		String[] args = { "self_instance", "-s", "/sdcard/self/etc/", "-i", "/sdcard/self/", "-f", "0" };

		System.loadLibrary("self_android");

		if (IsRunning() == 0) {
			if (Start(args) == 0)
				tv.setText("Self running...");
			else
				tv.setText("Failed to start self...");
		} else
			tv.setText("Self already running...");

		setContentView(tv);
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
		if (IsRunning() != 0)
			Stop();
	}

	public native int Start(String[] a_Args);

	public native int IsRunning();

	public native int Stop();
}
