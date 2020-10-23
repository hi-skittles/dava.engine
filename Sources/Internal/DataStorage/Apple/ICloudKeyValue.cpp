#include "ICloudKeyValue.h"
#include "Logger/Logger.h"

#if defined(__DAVAENGINE_APPLE__) && !defined(__DAVAENGINE_STEAM__)

#import "Utils/NSStringUtils.h"

namespace DAVA
{
ICloudKeyValue::ICloudKeyValue()
{
    Sync();
}

String ICloudKeyValue::GetStringValue(const String& key)
{
    Logger::FrameworkDebug("Trying to Get String value for %s key", key.c_str());

    NSUbiquitousKeyValueStore* iCloudStorage = [NSUbiquitousKeyValueStore defaultStore];
    NSString* value = [iCloudStorage stringForKey:NSStringFromString(key)];

    if (nil != value)
    {
        return String([value UTF8String]);
    }

    return "";
}

int64 ICloudKeyValue::GetLongValue(const String& key)
{
    Logger::FrameworkDebug("Trying to Get Long value for %s key", key.c_str());

    NSUbiquitousKeyValueStore* iCloudStorage = [NSUbiquitousKeyValueStore defaultStore];
    int64 value = [iCloudStorage longLongForKey:NSStringFromString(key)];

    return value;
}

void ICloudKeyValue::SetStringValue(const String& key, const String& value)
{
    Logger::FrameworkDebug("Trying to set %s value for %s key", value.c_str(), key.c_str());

    NSUbiquitousKeyValueStore* iCloudStorage = [NSUbiquitousKeyValueStore defaultStore];
    [iCloudStorage setString:NSStringFromString(value) forKey:NSStringFromString(key)];
}

void ICloudKeyValue::SetLongValue(const String& key, int64 value)
{
    Logger::FrameworkDebug("Trying to set long %lld value for %s key", value, key.c_str());

    NSUbiquitousKeyValueStore* iCloudStorage = [NSUbiquitousKeyValueStore defaultStore];
    [iCloudStorage setLongLong:value forKey:NSStringFromString(key)];
}

void ICloudKeyValue::RemoveEntry(const String& key)
{
    NSUbiquitousKeyValueStore* iCloudStorage = [NSUbiquitousKeyValueStore defaultStore];
    [iCloudStorage removeObjectForKey:NSStringFromString(key)];
}

void ICloudKeyValue::Clear()
{
    NSUbiquitousKeyValueStore* iCloudStorage = [NSUbiquitousKeyValueStore defaultStore];
    Sync();
    NSDictionary* dict = [iCloudStorage dictionaryRepresentation];
    NSArray* arr = [dict allKeys];

    for (uint32 i = 0; i < static_cast<uint32>(arr.count); i++)
    {
        NSString* key = [arr objectAtIndex:i];
        [iCloudStorage removeObjectForKey:key];
    }
}

void ICloudKeyValue::Push()
{
    Sync();
}

void ICloudKeyValue::Sync()
{
    NSUbiquitousKeyValueStore* iCloudStorage = [NSUbiquitousKeyValueStore defaultStore];
    [iCloudStorage synchronize];
}
}

#endif
