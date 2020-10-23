#include "RegKey.h"

using namespace DAVA;

RegKey::RegKey(HKEY scope, const wchar_t* keyName, bool createIfNotExist)
{
    long res = ::RegOpenKeyExW(scope, keyName, 0, KEY_READ | KEY_WOW64_64KEY, &key);

    if (res != ERROR_SUCCESS && createIfNotExist)
    {
        res = ::RegCreateKeyExW(
        scope, keyName, 0, 0, 0, KEY_WRITE | KEY_WOW64_64KEY, 0, &key, 0);
        isCreated = res == ERROR_SUCCESS;
    }

    isExist = res == ERROR_SUCCESS;
}

WideString RegKey::QueryString(const wchar_t* valueName) const
{
    Array<wchar_t, 1024> arr{};
    DWORD size = static_cast<DWORD>(arr.size());
    DWORD type;

    ::RegQueryValueExW(key,
                       valueName,
                       NULL,
                       &type,
                       reinterpret_cast<LPBYTE>(arr.data()),
                       &size);

    return type == REG_SZ ? arr.data() : L"";
}

bool RegKey::SetValue(const WideString& valName, const WideString& val)
{
    long res = ::RegSetValueExW(key, valName.c_str(), 0, REG_SZ,
                                (LPBYTE)val.c_str(), static_cast<DWORD>(val.size() + 1));
    return res == ERROR_SUCCESS;
}

DWORD RegKey::QueryDWORD(const wchar_t* valueName) const
{
    DWORD result;
    DWORD size = sizeof(result);
    DWORD type;

    ::RegQueryValueExW(key,
                       valueName,
                       NULL,
                       &type,
                       reinterpret_cast<LPBYTE>(&result),
                       &size);

    return type == REG_DWORD ? result : -1;
}

bool RegKey::SetValue(const WideString& valName, DWORD val)
{
    long res = ::RegSetValueExW(key, valName.c_str(), 0, REG_DWORD,
                                (LPBYTE)&val, sizeof(DWORD));
    return res == ERROR_SUCCESS;
}