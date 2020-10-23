#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_ANDROID__)

#include "Engine/Engine.h"
#include "Utils/Utils.h"
#include "DeviceInfoAndroid.h"
#include "Render/Renderer.h"
#include <unistd.h>

extern "C"
{

JNIEXPORT void JNICALL Java_com_dava_engine_PhoneStateListener_OnCarrierNameChanged(JNIEnv* env, jobject jclazz)
{
    DAVA::RunOnMainThreadAsync([]() {
        DAVA::DeviceInfo::carrierNameChanged.Emit(DAVA::DeviceInfo::GetCarrierName());
    });
}
}

namespace DAVA
{
DeviceInfoPrivate::DeviceInfoPrivate()
    : jniDeviceInfo("com/dava/engine/DeviceInfo")
{
    jgetVersion = jniDeviceInfo.GetStaticMethod<jstring>("GetVersion");
    jgetManufacturer = jniDeviceInfo.GetStaticMethod<jstring>("GetManufacturer");
    jgetModel = jniDeviceInfo.GetStaticMethod<jstring>("GetModel");
    jgetLocale = jniDeviceInfo.GetStaticMethod<jstring>("GetLocale");
    jgetRegion = jniDeviceInfo.GetStaticMethod<jstring>("GetRegion");
    jgetTimeZone = jniDeviceInfo.GetStaticMethod<jstring>("GetTimeZone");
    jgetUDID = jniDeviceInfo.GetStaticMethod<jstring>("GetUDID");
    jgetName = jniDeviceInfo.GetStaticMethod<jstring>("GetName");
    jgetZBufferSize = jniDeviceInfo.GetStaticMethod<jint>("GetZBufferSize");
    jgetHTTPProxyHost = jniDeviceInfo.GetStaticMethod<jstring>("GetHTTPProxyHost");
    jgetHTTPNonProxyHosts = jniDeviceInfo.GetStaticMethod<jstring>("GetHTTPNonProxyHosts");
    jgetHTTPProxyPort = jniDeviceInfo.GetStaticMethod<jint>("GetHTTPProxyPort");
    jgetNetworkType = jniDeviceInfo.GetStaticMethod<jint>("GetNetworkType");
    jgetSignalStrength = jniDeviceInfo.GetStaticMethod<jint, jint>("GetSignalStrength");
    jisPrimaryExternalStoragePresent = jniDeviceInfo.GetStaticMethod<jboolean>("IsPrimaryExternalStoragePresent");
    jgetCarrierName = jniDeviceInfo.GetStaticMethod<jstring>("GetCarrierName");
    jgetGpuFamily = jniDeviceInfo.GetStaticMethod<jbyte>("GetGpuFamily");
}

DeviceInfo::ePlatform DeviceInfoPrivate::GetPlatform()
{
    return DeviceInfo::PLATFORM_ANDROID;
}

String DeviceInfoPrivate::GetPlatformString()
{
    return GlobalEnumMap<DeviceInfo::ePlatform>::Instance()->ToString(GetPlatform());
}

String DeviceInfoPrivate::GetVersion()
{
    return JNI::JavaStringToString(JNI::LocalRef<jstring>(jgetVersion()));
}

String DeviceInfoPrivate::GetManufacturer()
{
    return JNI::JavaStringToString(JNI::LocalRef<jstring>(jgetManufacturer()));
}

String DeviceInfoPrivate::GetModel()
{
    return JNI::JavaStringToString(JNI::LocalRef<jstring>(jgetModel()));
}

String DeviceInfoPrivate::GetLocale()
{
    return JNI::JavaStringToString(JNI::LocalRef<jstring>(jgetLocale()));
}

String DeviceInfoPrivate::GetRegion()
{
    return JNI::JavaStringToString(JNI::LocalRef<jstring>(jgetRegion()));
}

String DeviceInfoPrivate::GetTimeZone()
{
    return JNI::JavaStringToString(JNI::LocalRef<jstring>(jgetTimeZone()));
}

String DeviceInfoPrivate::GetUDID()
{
    return JNI::JavaStringToString(JNI::LocalRef<jstring>(jgetUDID()));
}

WideString DeviceInfoPrivate::GetName()
{
    return JNI::JavaStringToWideString(JNI::LocalRef<jstring>(jgetName()));
}

int32 DeviceInfoPrivate::GetZBufferSize()
{
    return static_cast<int32>(jgetZBufferSize());
}

String DeviceInfoPrivate::GetHTTPProxyHost()
{
    return JNI::JavaStringToString(JNI::LocalRef<jstring>(jgetHTTPProxyHost()));
}

String DeviceInfoPrivate::GetHTTPNonProxyHosts()
{
    return JNI::JavaStringToString(JNI::LocalRef<jstring>(jgetHTTPNonProxyHosts()));
}

int32 DeviceInfoPrivate::GetHTTPProxyPort()
{
    return static_cast<int32>(jgetHTTPProxyPort());
}

eGPUFamily DeviceInfoPrivate::GetGPUFamilyImpl()
{
    eGPUFamily gpuFamily = static_cast<eGPUFamily>(jgetGpuFamily());
    return gpuFamily;
}

DeviceInfo::NetworkInfo DeviceInfoPrivate::GetNetworkInfo()
{
    DeviceInfo::NetworkInfo info;
    info.networkType = static_cast<DeviceInfo::eNetworkType>(GetNetworkType());
    info.signalStrength = GetSignalStrength(info.networkType);
    return info;
}

List<DeviceInfo::StorageInfo> DeviceInfoPrivate::GetStoragesList()
{
    List<DeviceInfo::StorageInfo> l;

    DeviceInfo::StorageInfo internal = GetInternalStorageInfo();
    DeviceInfo::StorageInfo external = GetPrimaryExternalStorageInfo();
    List<DeviceInfo::StorageInfo> secondaryList = GetSecondaryExternalStoragesList();

    if (internal.type != DeviceInfo::STORAGE_TYPE_UNKNOWN)
    {
        l.push_back(internal);
    }
    if (external.type != DeviceInfo::STORAGE_TYPE_UNKNOWN)
    {
        l.push_back(external);
    }

    std::copy(secondaryList.begin(), secondaryList.end(), back_inserter(l));

    return l;
}

bool DeviceInfoPrivate::IsHIDConnected(DeviceInfo::eHIDType type)
{
    //TODO: remove this empty realization and implement detection of HID connection
    return DeviceInfo::eHIDType::HID_TOUCH_TYPE == type;
}

bool DeviceInfoPrivate::IsTouchPresented()
{
    //TODO: remove this empty realization and implement detection touch
    return true;
}

DeviceInfo::StorageInfo DeviceInfoPrivate::StorageInfoFromJava(jobject object)
{
    DeviceInfo::StorageInfo info;

    if (object)
    {
        JNIEnv* env = JNI::GetEnv();
        JNI::LocalRef<jclass> classInfo = env->GetObjectClass(object);

        jfieldID fieldID;

        fieldID = env->GetFieldID(classInfo, "freeSpace", "J");
        info.freeSpace = env->GetLongField(object, fieldID);

        fieldID = env->GetFieldID(classInfo, "capacity", "J");
        info.totalSpace = env->GetLongField(object, fieldID);

        fieldID = env->GetFieldID(classInfo, "readOnly", "Z");
        info.readOnly = env->GetBooleanField(object, fieldID);

        fieldID = env->GetFieldID(classInfo, "removable", "Z");
        info.removable = env->GetBooleanField(object, fieldID);

        fieldID = env->GetFieldID(classInfo, "emulated", "Z");
        info.emulated = env->GetBooleanField(object, fieldID);

        fieldID = env->GetFieldID(classInfo, "path", "Ljava/lang/String;");
        JNI::LocalRef<jstring> jStr = static_cast<jstring>(env->GetObjectField(object, fieldID));

        info.path = JNI::JavaStringToString(jStr);
    }

    return info;
}

int32 DeviceInfoPrivate::GetNetworkType()
{
    return jgetNetworkType();
}

int32 DeviceInfoPrivate::GetSignalStrength(int32 networkType)
{
    return jgetSignalStrength(networkType);
}

DeviceInfo::StorageInfo DeviceInfoPrivate::GetInternalStorageInfo()
{
    JNIEnv* env = JNI::GetEnv();
    jmethodID mid = env->GetStaticMethodID(jniDeviceInfo, "GetInternalStorageInfo", "()Lcom/dava/engine/DeviceInfo$StorageInfo;");

    DeviceInfo::StorageInfo info;

    if (mid)
    {
        JNI::LocalRef<jobject> object = static_cast<jobject>(env->CallStaticObjectMethod(jniDeviceInfo, mid));
        DAVA_JNI_EXCEPTION_CHECK();
        if (object)
        {
            info = StorageInfoFromJava(object);
            info.type = DeviceInfo::STORAGE_TYPE_INTERNAL;
        }
    }

    return info;
}

bool DeviceInfoPrivate::IsPrimaryExternalStoragePresent()
{
    return jisPrimaryExternalStoragePresent();
}

DeviceInfo::StorageInfo DeviceInfoPrivate::GetPrimaryExternalStorageInfo()
{
    DeviceInfo::StorageInfo info;
    if (!IsPrimaryExternalStoragePresent())
    {
        return info;
    }

    JNIEnv* env = JNI::GetEnv();

    jmethodID mid = env->GetStaticMethodID(jniDeviceInfo, "GetPrimaryExternalStorageInfo", "()Lcom/dava/engine/DeviceInfo$StorageInfo;");

    if (mid)
    {
        JNI::LocalRef<jobject> object = static_cast<jobject>(env->CallStaticObjectMethod(jniDeviceInfo, mid));
        DAVA_JNI_EXCEPTION_CHECK();
        if (object)
        {
            info = StorageInfoFromJava(object);
            info.type = DeviceInfo::STORAGE_TYPE_PRIMARY_EXTERNAL;
        }
    }

    return info;
}

List<DeviceInfo::StorageInfo> DeviceInfoPrivate::GetSecondaryExternalStoragesList()
{
    List<DeviceInfo::StorageInfo> list;

    JNIEnv* env = JNI::GetEnv();

    jmethodID mid = env->GetStaticMethodID(jniDeviceInfo, "GetSecondaryExternalStoragesList", "()[Lcom/dava/engine/DeviceInfo$StorageInfo;");

    if (mid)
    {
        JNI::LocalRef<jarray> array = static_cast<jarray>(env->CallStaticObjectMethod(jniDeviceInfo, mid));
        DAVA_JNI_EXCEPTION_CHECK();
        if (array)
        {
            jsize length = env->GetArrayLength(array);

            for (jsize i = 0; i < length; ++i)
            {
                JNI::LocalRef<jobject> object = env->GetObjectArrayElement(static_cast<jobjectArray>(array.Get()), i);

                if (object)
                {
                    DeviceInfo::StorageInfo info = StorageInfoFromJava(object);
                    info.type = DeviceInfo::STORAGE_TYPE_SECONDARY_EXTERNAL;

                    list.push_back(info);
                }
            }
        }
    }

    return list;
}

String DeviceInfoPrivate::GetCarrierName()
{
    return JNI::JavaStringToString(JNI::LocalRef<jstring>(jgetCarrierName()));
}
}

#endif
