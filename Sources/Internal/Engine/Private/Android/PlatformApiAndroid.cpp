#include "Engine/PlatformApiAndroid.h"

#if defined(__DAVAENGINE_ANDROID__)

#include "Engine/Private/Android/AndroidBridge.h"
#include "Engine/Private/Android/WindowImplAndroid.h"
#include "Engine/Private/EngineBackend.h"
#include "Logger/Logger.h"
#include "Utils/UTF8Utils.h"

namespace DAVA
{
namespace PlatformApi
{
namespace Android
{
jobject CreateNativeControl(Window* targetWindow, const char8* controlClassName, void* backendPointer)
{
    using namespace DAVA::Private;
    WindowImpl* wb = EngineBackend::GetWindowImpl(targetWindow);
    return wb->CreateNativeControl(controlClassName, backendPointer);
}

} // namespace Android
} // namespace PlatformApi

namespace JNI
{
JNIEnv* GetEnv(bool abortIfNotAttachedToJVM)
{
    JNIEnv* env = Private::AndroidBridge::GetEnv();
    if (env == nullptr)
    {
        ANDROID_LOG_FATAL("Thread is not attached to Java VM");
        if (abortIfNotAttachedToJVM)
        {
            std::abort();
        }
    }
    return env;
}

void AttachCurrentThreadToJVM()
{
    Private::AndroidBridge::AttachCurrentThreadToJavaVM();
}

void DetachCurrentThreadFromJVM()
{
    Private::AndroidBridge::DetachCurrentThreadFromJavaVM();
}

bool CheckJavaException(JNIEnv* env, bool throwJniException)
{
    jthrowable e = env->ExceptionOccurred();
    if (e != nullptr)
    {
#if defined(__DAVAENGINE_DEBUG__)
        env->ExceptionDescribe();
#endif
        env->ExceptionClear();

        String exceptionText = GetJavaExceptionText(env, e);
        if (throwJniException)
        {
            throw Exception(exceptionText);
        }
        else
        {
            // Use native android logging mechanism as DAVA::Logger may not be constructed yet
            ANDROID_LOG_ERROR("[java exception] %s", exceptionText.c_str());
        }
        return true;
    }
    return false;
}

String GetJavaExceptionText(JNIEnv* env, jthrowable e)
{
    return Private::AndroidBridge::toString(env, e);
}

jclass LoadJavaClass(const char8* className, bool throwJniException, JNIEnv* env)
{
    if (env == nullptr)
    {
        env = GetEnv();
    }

    if (env != nullptr)
    {
        return Private::AndroidBridge::LoadJavaClass(env, className, throwJniException);
    }
    return nullptr;
}

String JavaStringToString(jstring string, JNIEnv* env)
{
    String result;
    if (string != nullptr)
    {
        if (env == nullptr)
        {
            env = GetEnv();
        }

        if (env != nullptr)
        {
            LocalRef<jbyteArray> bytes = Private::AndroidBridge::JavaStringToUtf8Bytes(env, string);
            if (bytes != nullptr)
            {
                const jsize length = env->GetArrayLength(bytes);
                jbyte* pBytes = env->GetByteArrayElements(bytes, nullptr);
                JNI::CheckJavaException(env, true);
                if (pBytes != nullptr)
                {
                    result.assign(reinterpret_cast<char*>(pBytes), reinterpret_cast<char*>(pBytes + length));
                    env->ReleaseByteArrayElements(bytes, pBytes, JNI_ABORT);
                    JNI::CheckJavaException(env, true);
                }
            }
        }
    }
    return result;
}

WideString JavaStringToWideString(jstring string, JNIEnv* env)
{
    return UTF8Utils::EncodeToWideString(JavaStringToString(string, env));
}

jstring CStrToJavaString(const char* cstr, JNIEnv* env)
{
    if (cstr != nullptr)
    {
        if (env == nullptr)
        {
            env = GetEnv();
        }

        if (env != nullptr)
        {
            jsize length = static_cast<jsize>(strlen(cstr));
            LocalRef<jbyteArray> bytes = env->NewByteArray(length);
            JNI::CheckJavaException(env, true);
            env->SetByteArrayRegion(bytes, 0, length, reinterpret_cast<const jbyte*>(cstr));
            JNI::CheckJavaException(env, true);
            return Private::AndroidBridge::JavaStringFromUtf8Bytes(env, bytes);
        }
    }
    return nullptr;
}

jstring StringToJavaString(const String& string, JNIEnv* env)
{
    return CStrToJavaString(string.c_str(), env);
}

jstring WideStringToJavaString(const WideString& string, JNIEnv* env)
{
    return CStrToJavaString(UTF8Utils::EncodeToUTF8(string).c_str(), env);
}

} // namespace JNI
} // namespace DAVA

#endif // __DAVAENGINE_ANDROID__
