package com.dava.testbed;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.content.Intent;
import android.content.res.Configuration;

import com.dava.engine.DavaActivity;

// Class for testing com.dava.engine.BootClasses meta-tag specified in AndroidManifest.xml
public class TestBed extends DavaActivity.ActivityListenerImpl
{
    public TestBed()
    {
        Log.d(DavaActivity.LOG_TAG, "TestBed.TestBed");
        DavaActivity.instance().registerActivityListener(this);
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        Log.d(DavaActivity.LOG_TAG, "TestBed.onCreate");
    }

    @Override
    public void onStart()
    {
        Log.d(DavaActivity.LOG_TAG, "TestBed.onStart");
    }

    @Override
    public void onResume()
    {
        Log.d(DavaActivity.LOG_TAG, "TestBed.onResume");
    }

    @Override
    public void onPause()
    {
        Log.d(DavaActivity.LOG_TAG, "TestBed.onPause");
    }

    @Override
    public void onRestart()
    {
        Log.d(DavaActivity.LOG_TAG, "TestBed.onRestart");
    }

    @Override
    public void onStop()
    {
        Log.d(DavaActivity.LOG_TAG, "TestBed.onStop");
    }

    @Override
    public void onDestroy()
    {
        Log.d(DavaActivity.LOG_TAG, "TestBed.onDestroy");
        DavaActivity.instance().unregisterActivityListener(this);
    }

    @Override
    public void onWindowFocusChanged(boolean hasWindowFocus)
    {
        Log.d(DavaActivity.LOG_TAG, "TestBed.onWindowFocusChanged");
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig)
    {
        Log.d(DavaActivity.LOG_TAG, "TestBed.onConfigurationChanged");
    }

    @Override
    public void onBackPressed()
    {
        Log.d(DavaActivity.LOG_TAG, "TestBed.onBackPressed");
    }

	@Override
	public void onSaveInstanceState(Bundle outState)
	{
		Log.d(DavaActivity.LOG_TAG, "TestBed.onSaveInstanceState");	
	}

	@Override
	public void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		Log.d(DavaActivity.LOG_TAG, "TestBed.onActivityResult");
	}

	@Override
	public void onNewIntent(Intent intent)
	{
		Log.d(DavaActivity.LOG_TAG, "TestBed.onNewIntent");	
	}

	@Override
	public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults)
	{
		Log.d(DavaActivity.LOG_TAG, "TestBed.onRequestPermissionsResult");
	}
}
