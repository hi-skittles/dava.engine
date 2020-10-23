#include "Notification/Private/Android/LocalNotificationAndroid.h"

#if defined(__DAVAENGINE_ANDROID__)

#include "Concurrency/LockGuard.h"
#include "Engine/Engine.h"
#include "Engine/PlatformApiAndroid.h"
#include "Notification/LocalNotificationController.h"

namespace DAVA
{
LocalNotificationAndroid::LocalNotificationAndroid(const String& _id)
    : notificationProvider("com/dava/engine/notification/DavaNotificationProvider")
{
    notificationId = _id;

    setText = notificationProvider.GetStaticMethod<void, jstring, jstring, jstring, jboolean>("NotifyText");
    setProgress = notificationProvider.GetStaticMethod<void, jstring, jstring, jstring, jint, jint, jboolean>("NotifyProgress");
    hideNotification = notificationProvider.GetStaticMethod<void, jstring>("HideNotification");
    enableTapAction = notificationProvider.GetStaticMethod<void, jstring>("EnableTapAction");

    notifyDelayed = notificationProvider.GetStaticMethod<void, jstring, jstring, jstring, jint, jboolean>("NotifyDelayed");
    removeAllDelayedNotifications = notificationProvider.GetStaticMethod<void>("RemoveAllDelayedNotifications");
}

// TODO: Remove this method, after transition on Core V2.
void LocalNotificationAndroid::SetAction(const String& action)
{
    LockGuard<Mutex> mutexGuard(javaCallMutex);
    JNIEnv* env = JNI::GetEnv();
    jstring jstrNotificationUid = env->NewStringUTF(notificationId.c_str());
    enableTapAction(jstrNotificationUid);
    env->DeleteLocalRef(jstrNotificationUid);
}

void LocalNotificationAndroid::Hide()
{
    LockGuard<Mutex> mutexGuard(javaCallMutex);
    JNIEnv* env = JNI::GetEnv();
    jstring jstrNotificationUid = env->NewStringUTF(notificationId.c_str());
    hideNotification(jstrNotificationUid);
    env->DeleteLocalRef(jstrNotificationUid);
}

void LocalNotificationAndroid::ShowText(const String& title, const String& text, bool useSound)
{
    LockGuard<Mutex> mutexGuard(javaCallMutex);
    JNIEnv* env = JNI::GetEnv();

    jstring jstrNotificationUid = env->NewStringUTF(notificationId.c_str());

    jstring jStrTitle = JNI::StringToJavaString(title);
    jstring jStrText = JNI::StringToJavaString(text);

    setText(jstrNotificationUid, jStrTitle, jStrText, useSound);

    env->DeleteLocalRef(jstrNotificationUid);
    env->DeleteLocalRef(jStrTitle);
    env->DeleteLocalRef(jStrText);
}

void LocalNotificationAndroid::ShowProgress(const String& title, const String& text, const uint32 total, const uint32 progress, bool useSound)
{
    LockGuard<Mutex> mutexGuard(javaCallMutex);
    JNIEnv* env = JNI::GetEnv();

    jstring jstrNotificationUid = env->NewStringUTF(notificationId.c_str());

    jstring jStrTitle = JNI::StringToJavaString(title);
    jstring jStrText = JNI::StringToJavaString(text);

    setProgress(jstrNotificationUid, jStrTitle, jStrText, total, progress, useSound);

    env->DeleteLocalRef(jstrNotificationUid);
    env->DeleteLocalRef(jStrTitle);
    env->DeleteLocalRef(jStrText);
}

void LocalNotificationAndroid::PostDelayedNotification(const String& title, const String& text, int delaySeconds, bool useSound)
{
    LockGuard<Mutex> mutexGuard(javaCallMutex);
    JNIEnv* env = JNI::GetEnv();

    jstring jstrNotificationUid = env->NewStringUTF(notificationId.c_str());
    jstring jStrTitle = JNI::StringToJavaString(title);
    jstring jStrText = JNI::StringToJavaString(text);

    notifyDelayed(jstrNotificationUid, jStrTitle, jStrText, delaySeconds, useSound);

    env->DeleteLocalRef(jstrNotificationUid);
    env->DeleteLocalRef(jStrTitle);
    env->DeleteLocalRef(jStrText);
}

void LocalNotificationAndroid::RemoveAllDelayedNotifications()
{
    removeAllDelayedNotifications();
}

LocalNotificationImpl* LocalNotificationImpl::Create(const String& _id)
{
    return new LocalNotificationAndroid(_id);
}

void LocalNotificationImpl::RequestPermissions()
{
}
} // namespace DAVA

#endif // defined(__DAVAENGINE_ANDROID__)
