#pragma once

#include "Base/Platform.h"

#if defined(__DAVAENGINE_ANDROID__)

#include "Engine/PlatformApiAndroid.h"
#include "Base/BaseTypes.h"
#include "Platform/DeviceInfoPrivateBase.h"

namespace DAVA
{
class DeviceInfoPrivate : public DeviceInfoPrivateBase
{
public:
    DeviceInfoPrivate();
    DeviceInfo::ePlatform GetPlatform();
    String GetPlatformString();
    String GetVersion();
    String GetManufacturer();
    String GetModel();
    String GetLocale();
    String GetRegion();
    String GetTimeZone();
    String GetUDID();
    WideString GetName();
    String GetHTTPProxyHost();
    String GetHTTPNonProxyHosts();
    int32 GetHTTPProxyPort();
    int32 GetZBufferSize();
    eGPUFamily GetGPUFamilyImpl() override;
    DeviceInfo::NetworkInfo GetNetworkInfo();
    List<DeviceInfo::StorageInfo> GetStoragesList();
    bool IsHIDConnected(DeviceInfo::eHIDType type);
    bool IsTouchPresented();
    String GetCarrierName();

protected:
    DeviceInfo::StorageInfo StorageInfoFromJava(jobject object);

private:
    int32 GetNetworkType();
    int32 GetSignalStrength(int32 networkType);
    bool IsPrimaryExternalStoragePresent();
    DeviceInfo::StorageInfo GetInternalStorageInfo();
    DeviceInfo::StorageInfo GetPrimaryExternalStorageInfo();
    List<DeviceInfo::StorageInfo> GetSecondaryExternalStoragesList();

    JNI::JavaClass jniDeviceInfo;
    Function<jstring()> jgetVersion;
    Function<jstring()> jgetManufacturer;
    Function<jstring()> jgetModel;
    Function<jstring()> jgetLocale;
    Function<jstring()> jgetRegion;
    Function<jstring()> jgetTimeZone;
    Function<jstring()> jgetUDID;
    Function<jstring()> jgetName;
    Function<jint()> jgetZBufferSize;
    Function<jstring()> jgetHTTPProxyHost;
    Function<jstring()> jgetHTTPNonProxyHosts;
    Function<jint()> jgetHTTPProxyPort;
    Function<jint()> jgetNetworkType;
    Function<jint(jint)> jgetSignalStrength;
    Function<jboolean()> jisPrimaryExternalStoragePresent;
    Function<jstring()> jgetCarrierName;
    Function<jbyte()> jgetGpuFamily;
};
};

#endif //defined(__DAVAENGINE_ANDROID__)
