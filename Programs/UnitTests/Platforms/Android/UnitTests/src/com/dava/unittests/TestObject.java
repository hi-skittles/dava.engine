package com.dava.unittests;

public class TestObject
{
    public int field = 0;
    public static long staticField = 0;

    TestObject()
    {
        staticField += 1;
    }
    TestObject(int v)
    {
        field = v;
        staticField += 1;
    }
}
