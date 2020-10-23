#pragma once

#include "Base/EnumMap.h"

template <typename T>
class GlobalEnumMap
{
public:
    explicit GlobalEnumMap();
    ~GlobalEnumMap();

    static const EnumMap* Instance();

protected:
    static void RegisterAll();
    static void Register(const int e, const char* s);
};

template <typename T>
const EnumMap* GlobalEnumMap<T>::Instance()
{
    static EnumMap enumMap;
    static bool initialized = false;

    if (!initialized)
    {
        initialized = true;
        RegisterAll();
    }

    return &enumMap;
}

template <typename T>
void GlobalEnumMap<T>::Register(const int e, const char* s)
{
    Instance()->Register(e, s);
}

#define ENUM_DECLARE(eType) template <> void GlobalEnumMap<eType>::RegisterAll()
#define ENUM_ADD(eValue) Register(static_cast<int>(eValue), #eValue)
#define ENUM_ADD_DESCR(eValue, eDescr) Register(static_cast<int>(eValue), eDescr)

// Define:
//	ENUM_DECLARE(AnyEnumType)
//	{
//		ENUM_ADDS(AnyEnumType::Value1);
//		ENUM_ADD_DESCR(AnyEnumType::Value2, "Value2");
//	}
//
// Usage:
//  GlobalEnumMap::Instance<AnyEnumType>();
