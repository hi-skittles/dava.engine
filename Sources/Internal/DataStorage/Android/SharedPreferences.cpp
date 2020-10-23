#include "Logger/Logger.h"
#include "SharedPreferences.h"

namespace DAVA
{

#if defined(__DAVAENGINE_ANDROID__)

SharedPreferences::SharedPreferences()
    : jniSharedPreferences("com/dava/engine/SharedPreferences")
{
    getSharedPreferences = jniSharedPreferences.GetStaticMethod<jobject>("GetSharedPreferences");

    JNIEnv* env = JNI::GetEnv();
    jobject tmp = getSharedPreferences();
    preferencesObject = env->NewGlobalRef(tmp);
    env->DeleteLocalRef(tmp);

    auto tempPutString = jniSharedPreferences.GetMethod<void, jstring, jstring>("PutString");
    putString = Bind(tempPutString, preferencesObject, _1, _2);

    auto tempGetString = jniSharedPreferences.GetMethod<jstring, jstring, jstring>("GetString");
    getString = Bind(tempGetString, preferencesObject, _1, _2);

    auto tempPutLong = jniSharedPreferences.GetMethod<void, jstring, jlong>("PutLong");
    putLong = Bind(tempPutLong, preferencesObject, _1, _2);

    auto tempGetLong = jniSharedPreferences.GetMethod<jlong, jstring, jlong>("GetLong");
    getLong = Bind(tempGetLong, preferencesObject, _1, _2);

    auto tempRemove = jniSharedPreferences.GetMethod<void, jstring>("Remove");
    remove = Bind(tempRemove, preferencesObject, _1);

    auto tempClear = jniSharedPreferences.GetMethod<void>("Clear");
    clear = Bind(tempClear, preferencesObject);

    auto tempPush = jniSharedPreferences.GetMethod<void>("Push");
    push = Bind(tempPush, preferencesObject);
}

SharedPreferences::~SharedPreferences()
{
    JNI::GetEnv()->DeleteGlobalRef(preferencesObject);
}

String SharedPreferences::GetStringValue(const String& key)
{
    Logger::FrameworkDebug("Trying to Get String value for %s key", key.c_str());

    JNIEnv* env = JNI::GetEnv();

    jstring jkey = env->NewStringUTF(key.c_str());
    jstring jdefvalue = env->NewStringUTF("");

    jstring jvalue = getString(jkey, jdefvalue);

    String retValue = JNI::ToString(jvalue);

    env->DeleteLocalRef(jkey);
    env->DeleteLocalRef(jdefvalue);
    env->DeleteLocalRef(jvalue);

    return retValue;
}

int64 SharedPreferences::GetLongValue(const String& key)
{
    Logger::FrameworkDebug("Trying to Get Long value for %s key", key.c_str());

    JNIEnv* env = JNI::GetEnv();

    jstring jkey = env->NewStringUTF(key.c_str());

    int64 retValue = static_cast<int64>(getLong(jkey, 0));

    env->DeleteLocalRef(jkey);

    return retValue;
}

void SharedPreferences::SetStringValue(const String& key, const String& value)
{
    Logger::FrameworkDebug("Trying to set string %s value for %s key", value.c_str(), key.c_str());
    JNIEnv* env = JNI::GetEnv();

    jstring jkey = env->NewStringUTF(key.c_str());
    jstring jvalue = env->NewStringUTF(value.c_str());

    putString(jkey, jvalue);

    env->DeleteLocalRef(jkey);
    env->DeleteLocalRef(jvalue);
}

void SharedPreferences::SetLongValue(const String& key, int64 value)
{
    Logger::FrameworkDebug("Trying to set long %lld value for %s key", value, key.c_str());
    JNIEnv* env = JNI::GetEnv();

    jstring jkey = env->NewStringUTF(key.c_str());

    putLong(jkey, value);

    env->DeleteLocalRef(jkey);
}

void SharedPreferences::RemoveEntry(const String& key)
{
    JNIEnv* env = JNI::GetEnv();
    jstring jkey = env->NewStringUTF(key.c_str());
    remove(jkey);
    env->DeleteLocalRef(jkey);
}

void SharedPreferences::Clear()
{
    clear();
}

void SharedPreferences::Push()
{
    push();
}

#endif
}
