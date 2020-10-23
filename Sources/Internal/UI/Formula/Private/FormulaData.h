#pragma once

#include "Base/Any.h"
#include "Base/BaseTypes.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
/**
 \ingroup formula

 FormulaDataVector and FormulaDataMap containers of any values. Used reflection
 structure wrappers to organize more simpler access to data.
 E.g. data.value[4] instead of data._vector_.value._map_[4]
 */
class FormulaDataVector : public ReflectionBase
{
    DAVA_VIRTUAL_REFLECTION(DataVectorAny);

public:
    FormulaDataVector();
    ~FormulaDataVector();

    bool IsEmpty() const;
    void Add(const Any& value);
    Any& Get(size_t index);
    size_t GetCount() const;

private:
    Vector<Any> vector;
};

class FormulaDataMap : public ReflectionBase
{
    DAVA_VIRTUAL_REFLECTION(DataMapAny);

public:
    FormulaDataMap();
    ~FormulaDataMap();

    bool IsEmpty() const;
    Any& Find(const String& key);

    void Add(const String& key, const Any& value);

    const Vector<String>& GetOrderedKeys() const;

private:
    Any empty;
    Map<String, Any> map;
    Vector<String> orderedKeys;
};
}
