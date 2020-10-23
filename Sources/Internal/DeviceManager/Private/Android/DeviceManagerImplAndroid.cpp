#include "DeviceManager/Private/Android/DeviceManagerImplAndroid.h"

#if defined(__DAVAENGINE_ANDROID__)

#include "DeviceManager/DeviceManager.h"
#include "Engine/Private/Dispatcher/MainDispatcher.h"

DAVA_DECLARE_CUSTOM_JNI_TYPE(jDeviceManager, jobject, "Lcom/dava/engine/DeviceManager;");
DAVA_DECLARE_CUSTOM_JNI_TYPE(jDisplayInfoArray, jobjectArray, "[Lcom/dava/engine/DeviceManager$DisplayInfo;");

namespace DAVA
{
namespace Private
{
DeviceManagerImpl::DeviceManagerImpl(DeviceManager* devManager, Private::MainDispatcher* dispatcher)
    : deviceManager(devManager)
    , mainDispatcher(dispatcher)
    , javaDeviceManagerClass("com/dava/engine/DeviceManager")
    , javaDisplayInfoClass("com/dava/engine/DeviceManager$DisplayInfo")
{
    JNIEnv* env = JNI::GetEnv();

    // DeviceManager.getCpuTemperature()
    javaGetCpuTemperatureMethod = javaDeviceManagerClass.GetMethod<jfloat>("getCpuTemperature");

    // DeviceManager.instance()
    Function<jDeviceManager()> javaDeviceManagerGetInstanceMethod = javaDeviceManagerClass.GetStaticMethod<jDeviceManager>("instance");

    // Get Java DeviceManager instance
    javaDeviceManagerInstance = javaDeviceManagerGetInstanceMethod();

    // Handle displays

    // DeviceManager.getDisplaysInfo()
    Function<jDisplayInfoArray(jobject)> javaDeviceManagerGetDisplaysInfoMethod = javaDeviceManagerClass.GetMethod<jDisplayInfoArray>("getDisplaysInfo");

    // Save fields ids for Java DisplayInfo class
    javaDisplayInfoNameField = env->GetFieldID(javaDisplayInfoClass, "name", JNI::TypeSignature<jstring>::value());
    javaDisplayInfoIdField = env->GetFieldID(javaDisplayInfoClass, "id", JNI::TypeSignature<jint>::value());
    javaDisplayInfoWidthField = env->GetFieldID(javaDisplayInfoClass, "width", JNI::TypeSignature<jint>::value());
    javaDisplayInfoHeightField = env->GetFieldID(javaDisplayInfoClass, "height", JNI::TypeSignature<jint>::value());
    javaDisplayInfoDpiXField = env->GetFieldID(javaDisplayInfoClass, "dpiX", JNI::TypeSignature<jfloat>::value());
    javaDisplayInfoDpiYField = env->GetFieldID(javaDisplayInfoClass, "dpiY", JNI::TypeSignature<jfloat>::value());
    javaDisplayInfoMaxFps = env->GetFieldID(javaDisplayInfoClass, "maxFps", JNI::TypeSignature<jfloat>::value());

    // Get displays
    const jDisplayInfoArray displaysInfo = javaDeviceManagerGetDisplaysInfoMethod(javaDeviceManagerInstance);

    // Convert & save to C++ DeviceManager
    const size_t displaysCount = static_cast<size_t>(env->GetArrayLength(displaysInfo));
    deviceManager->displays.resize(displaysCount);
    for (size_t i = 0; i < displaysCount; ++i)
    {
        const jobject javaDisplayInfo = env->GetObjectArrayElement(displaysInfo, i);

        // Java class should always put primary display into first position
        const bool isPrimary = (i == 0);
        deviceManager->displays[i] = ConvertFromJavaDisplayInfo(env, javaDisplayInfo, isPrimary);

        env->DeleteLocalRef(javaDisplayInfo);
    }

    env->DeleteLocalRef(displaysInfo);
}

void DeviceManagerImpl::UpdateDisplayConfig()
{
    // TODO: implement tracking display changes
}

DisplayInfo DeviceManagerImpl::ConvertFromJavaDisplayInfo(JNIEnv* env, const jobject javaDisplayInfo, const bool isPrimary)
{
    DisplayInfo displayInfo;

    jstring nameJava = static_cast<jstring>(env->GetObjectField(javaDisplayInfo, javaDisplayInfoNameField));
    displayInfo.name = JNI::ToString(nameJava);
    env->DeleteLocalRef(nameJava);

    displayInfo.systemId = static_cast<uintptr_t>(env->GetIntField(javaDisplayInfo, javaDisplayInfoIdField));
    displayInfo.rect.x = 0; // No information available from Android API for both x and y values
    displayInfo.rect.y = 0;
    displayInfo.rect.dx = static_cast<float32>(env->GetIntField(javaDisplayInfo, javaDisplayInfoWidthField));
    displayInfo.rect.dy = static_cast<float32>(env->GetIntField(javaDisplayInfo, javaDisplayInfoHeightField));
    displayInfo.rawDpiX = static_cast<float32>(env->GetFloatField(javaDisplayInfo, javaDisplayInfoDpiXField));
    displayInfo.rawDpiY = static_cast<float32>(env->GetFloatField(javaDisplayInfo, javaDisplayInfoDpiYField));
    displayInfo.maxFps = static_cast<int32>(env->GetFloatField(javaDisplayInfo, javaDisplayInfoMaxFps));
    displayInfo.primary = isPrimary;
    return displayInfo;
}

float32 DeviceManagerImpl::GetCpuTemperature() const
{
    return static_cast<float32>(javaGetCpuTemperatureMethod(javaDeviceManagerInstance));
}

} // namespace Private
} // namespace DAVA

#endif // __DAVAENGINE_ANDROID__
