package com.dava.engine;

import java.util.Locale;

public class Localization {

	public static String GetLocale()
	{
		String locale = Locale.getDefault().toString();
		return locale;
	}
}
