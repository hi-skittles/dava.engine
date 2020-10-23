package com.dava.unittests;

public class UnitTests
{
    private static UnitTests instance = null;

    public native void nativeCall();

    public UnitTests()
    {
        instance = this;
    }

    public static UnitTests instance()
    {
        return instance;
    }

    public void doNativeCall()
    {
        nativeCall();
    }

    public String sum(int a, int b, int c)
    {
        return String.format("%d", a + b + c);
    }

    public static String[] generateStringArray(int n, int start)
    {
        String[] v = new String[n];
        for (int i = 0;i < n;++i)
        {
            v[i] = String.format("%d", i + start);
        }
        return v;
    }

    public String[] updateStringArray(String[] v)
    {
        for (int i = 0;i < v.length;++i)
        {
            v[i] = String.format("%s-xray", v[i]);
        }
        return v;
    }
}
