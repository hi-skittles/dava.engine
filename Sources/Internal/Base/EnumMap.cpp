#include "Base/EnumMap.h"
#include "Debug/DVAssert.h"

EnumMap::EnumMap()
{
}

EnumMap::~EnumMap()
{
}

void EnumMap::Register(const int e, const char* s) const
{
    DVASSERT(!map.count(e));
    map[e] = DAVA::String(s);
    indexes.push_back(e);
}

void EnumMap::UnregistelAll() const
{
    map.clear();
    indexes.clear();
}

bool EnumMap::ToValue(const char* s, int& e) const
{
    bool ret = false;
    EnumMapContainer::const_iterator i = map.begin();
    EnumMapContainer::const_iterator end = map.end();
    DAVA::String str(s);

    for (; i != end; ++i)
    {
        if (i->second == str)
        {
            e = i->first;
            ret = true;
            break;
        }
    }

    return ret;
}

const char* EnumMap::ToString(const int e) const
{
    const char* ret = NULL;

    if (map.count(e))
    {
        ret = map.at(e).c_str();
    }
    else
    {
        DVASSERT(false, "Be sure that e is declared at global enum"); //example ENUM_ADD_DESCR(FORMAT_ATC_RGBA_EXPLICIT_ALPHA, "ATC_RGBA_EXPLICIT_ALPHA");
    }

    return ret;
}

size_t EnumMap::GetCount() const
{
    return map.size();
}

bool EnumMap::GetValue(size_t index, int& e) const
{
    bool ret = false;
    if (index < map.size())
    {
        e = indexes[index];
        ret = true;
    }

    return ret;
}
