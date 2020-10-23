#pragma once

#include "Base/BaseTypes.h"

class EnumMap
{
public:
    EnumMap();
    ~EnumMap();

    bool ToValue(const char*, int& e) const;
    const char* ToString(const int e) const;

    size_t GetCount() const;
    bool GetValue(size_t index, int& e) const;

    void Register(const int e, const char*) const;
    void UnregistelAll() const;

protected:
    using EnumMapContainer = DAVA::Map<int, DAVA::String>;
    mutable EnumMapContainer map;
    mutable DAVA::Vector<int> indexes;
};
