#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_ANDROID__)

#include <jni.h>
#include <android/log.h>

#include "Engine/Private/EnginePrivateFwd.h"

#define ANDROID_LOG_DEBUG(...) __android_log_print(ANDROID_LOG_DEBUG, "DAVA", __VA_ARGS__)
#define ANDROID_LOG_INFO(...) __android_log_print(ANDROID_LOG_INFO, "DAVA", __VA_ARGS__)
#define ANDROID_LOG_WARN(...) __android_log_print(ANDROID_LOG_WARN, "DAVA", __VA_ARGS__)
#define ANDROID_LOG_ERROR(...) __android_log_print(ANDROID_LOG_ERROR, "DAVA", __VA_ARGS__)
#define ANDROID_LOG_FATAL(...) __android_log_print(ANDROID_LOG_FATAL, "DAVA", __VA_ARGS__)

namespace DAVA
{
namespace Private
{
struct AndroidBridge final
{
    AndroidBridge(JavaVM* jvm);

    void InitializeJNI(JNIEnv* env);

    static JavaVM* GetJavaVM();
    static JNIEnv* GetEnv();
    static bool AttachCurrentThreadToJavaVM();
    static bool DetachCurrentThreadFromJavaVM();
    static void PostQuitToActivity();
    static jclass LoadJavaClass(JNIEnv* env, const char8* className, bool throwJniException);
    static String toString(JNIEnv* env, jobject object);
    static jbyteArray JavaStringToUtf8Bytes(JNIEnv* env, jstring string);
    static jstring JavaStringFromUtf8Bytes(JNIEnv* env, jbyteArray bytes);
    static String JavaStringToModifiedUtfString(JNIEnv* env, jstring string);
    static jstring JavaStringFromModifiedUtfString(JNIEnv* env, const char* cstr);

    static const String& GetExternalDocumentsDir();
    static const String& GetInternalDocumentsDir();
    static const String& GetApplicationPath();
    static const String& GetPackageName();

    static void HideSplashView();
    static void NotifyEngineRunning();

    static void AttachPlatformCore(PlatformCore* platformCore);

    void InitializeEngine(String externalFilesDir,
                          String internalFilesDir,
                          String sourceDir,
                          String apkName,
                          String cmdline);
    void ShutdownEngine();

    WindowImpl* ActivityOnCreate(JNIEnv* env, jobject activityInstance);
    void ActivityOnFileIntent(JNIEnv* env, jstring filename, jboolean onStartup);
    void ActivityOnResume();
    void ActivityOnPause();
    void ActivityOnDestroy(JNIEnv* env);
    void ActivityOnTrimMemory(jint level);

    void GameThread();

    void SetScreenTimeoutEnabled(bool enabled);

    JavaVM* javaVM = nullptr;
    jobject classLoader = nullptr; // Cached instance of ClassLoader
    jmethodID methodClassLoader_loadClass = nullptr; // ClassLoader.loadClass method
    jmethodID methodObject_toString = nullptr; // Object.toString method

    jclass classString = nullptr; // Class String
    jmethodID methodString_getBytes = nullptr; // String.getBytes method
    jmethodID methodString_initBytesCharset = nullptr; // Constructor java String(byte[] bytes, String charset)
    jstring constUtf8CharsetName = nullptr; // Java string with "UTF-8" charset name

    jobject activity = nullptr; // Reference to DavaActivity instance
    jmethodID methodDavaActivity_postFinish = nullptr; // DavaActivity.postFinish method
    jmethodID methodDavaActivity_hideSplashView = nullptr; // DavaActivity.hideSplashView method
    jmethodID methodDavaActivity_setScreenTimeoutEnabled = nullptr; // DavaActivity.setScreenTimeoutEnabled method
    jmethodID methodDavaActivity_notifyEngineRunning = nullptr; // DavaActivity.notifyEngineRunning method

    EngineBackend* engineBackend = nullptr;
    PlatformCore* core = nullptr;

    String externalDocumentsDir;
    String internalDocumentsDir;
    String appPath;
    String packageName;
    Vector<String> cmdargs;
};

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_ANDROID__
