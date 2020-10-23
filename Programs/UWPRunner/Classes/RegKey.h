#ifndef REGKEY_H
#define REGKEY_H

#include "Base/BaseTypes.h"
#include "Base/Platform.h"

class RegKey
{
public:
    RegKey(HKEY scope, const wchar_t* keyName, bool createIfNotExist = false);

    bool IsExist() const
    {
        return isExist;
    }
    bool IsCreated() const
    {
        return isCreated;
    }

    //TODO: replace on Optional<String>
    DAVA::WideString QueryString(const wchar_t* valueName) const;
    bool SetValue(const DAVA::WideString& valName, const DAVA::WideString& val);

    //TODO: replace on Optional<DWORD>
    DWORD QueryDWORD(const wchar_t* valueName) const;
    bool SetValue(const DAVA::WideString& valName, DWORD val);

    template <typename T>
    //TODO: replace on Optional<T>
    T QueryValue(const wchar_t* valueName);

private:
    bool isExist = false;
    bool isCreated = false;
    HKEY key;
};

template <>
inline DAVA::WideString RegKey::QueryValue<DAVA::WideString>(const wchar_t* valueName)
{
    return QueryString(valueName);
}

template <>
inline DWORD RegKey::QueryValue<DWORD>(const wchar_t* valueName)
{
    return QueryDWORD(valueName);
}

#endif // REGKEY_H