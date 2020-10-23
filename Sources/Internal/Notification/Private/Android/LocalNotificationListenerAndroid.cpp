#include "Notification/Private/Android/LocalNotificationListenerAndroid.h"

#if defined(__DAVAENGINE_ANDROID__)

#include "Engine/Engine.h"
#include "Engine/PlatformApiAndroid.h"
#include "Logger/Logger.h"
#include "Notification/LocalNotificationController.h"
#include "Notification/Private/Android/LocalNotificationAndroid.h"

extern "C"
{
JNIEXPORT void JNICALL Java_com_dava_engine_notification_LocalNotificationListener_nativeNewIntent(JNIEnv* env, jclass jclazz, jstring uid, jlong controller)
{
    DAVA::LocalNotificationController* localNotificationController = reinterpret_cast<DAVA::LocalNotificationController*>(static_cast<uintptr_t>(controller));
    DAVA::String uidStr = DAVA::JNI::JavaStringToString(uid);
    auto function = [uidStr, localNotificationController]()
    {
        localNotificationController->OnNotificationPressed(uidStr);
    };
    DAVA::RunOnMainThreadAsync(function);
}
} // extern "C"

namespace DAVA
{
namespace Private
{
LocalNotificationListener::LocalNotificationListener(LocalNotificationController& controller)
{
    try
    {
        JNIEnv* env = JNI::GetEnv();
        JNI::JavaClass clazz("com/dava/engine/notification/LocalNotificationListener");
        release = clazz.GetMethod<void>("release");
        jmethodID classConstructor = env->GetMethodID(clazz, "<init>", "(J)V");
        jobject obj = env->NewObject(clazz, classConstructor, reinterpret_cast<jlong>(&controller));
        instance = env->NewGlobalRef(obj);
        env->DeleteLocalRef(obj);
    }
    catch (const JNI::Exception& e)
    {
        Logger::Error("[LocalNotificationListener] failed to init java bridge: %s", e.what());
        DVASSERT(false, e.what());
        return;
    }
}

LocalNotificationListener::~LocalNotificationListener()
{
    if (instance != nullptr)
    {
        release(instance);
    }
}
} // namespace Private
} // namespace DAVA

#endif // defined(__DAVAENGINE_ANDROID__)
