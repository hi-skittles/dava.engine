package com.dava.engine;

public class SharedPreferences
{
	android.content.SharedPreferences preferences = null;
	android.content.SharedPreferences.Editor editor = null;
	static String name = "DavaPreferences";

	public static String GetName()
	{
		return name;
	}
	
	public static Object GetSharedPreferences()
	{
		SharedPreferences self = new SharedPreferences();
		return self;
	}

	public SharedPreferences()
	{
		DavaActivity activity = DavaActivity.instance();
		preferences = activity.getSharedPreferences(name, 0);
		editor = preferences.edit();
	}

	public String GetString(String key, String defaultValue)
	{	
		return preferences.getString(key, defaultValue); 
	}
	
	public long GetLong(String key, long defaultValue)
	{
		return preferences.getLong(key,  defaultValue);
	}
	
	public void PutString(String key, String value)
	{	
		editor.putString(key, value);
	}

	public void PutLong(String key, long value)
	{	
		editor.putLong(key, value);
	}
	
	public void Remove(String key)
	{
		editor.remove(key);
	}
	
	public void Clear()
	{
		editor.clear();
		editor.apply();
	}
	
	public void Push()
	{	
		editor.apply();
	}
}
