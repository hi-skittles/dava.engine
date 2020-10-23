#include "UI/Formula/Private/FormulaData.h"

#include "UI/Formula/Private/FormulaException.h"

#include "Debug/DVAssert.h"

#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
using std::shared_ptr;

FormulaDataVector::FormulaDataVector()
{
}

FormulaDataVector::~FormulaDataVector()
{
}

bool FormulaDataVector::IsEmpty() const
{
    return vector.empty();
}

void FormulaDataVector::Add(const Any& value)
{
    vector.push_back(value);
}

Any& FormulaDataVector::Get(size_t index)
{
    return vector.at(index);
}

size_t FormulaDataVector::GetCount() const
{
    return vector.size();
}

FormulaDataMap::FormulaDataMap()
{
}

FormulaDataMap::~FormulaDataMap()
{
}

bool FormulaDataMap::IsEmpty() const
{
    return map.empty();
}

Any& FormulaDataMap::Find(const String& key)
{
    auto it = map.find(key);
    if (it != map.end())
    {
        return it->second;
    }
    return empty;
}

void FormulaDataMap::Add(const String& key, const Any& value)
{
    map[key] = value;
    orderedKeys.push_back(key);
}

const Vector<String>& FormulaDataMap::GetOrderedKeys() const
{
    return orderedKeys;
}

class FormulaDataStructureWrapper : public StructureWrapperDefault
{
protected:
    Reflection CreateReflection(Any& val) const
    {
        if (val.IsEmpty())
        {
            return Reflection();
        }
        else if (val.CanGet<shared_ptr<FormulaDataMap>>())
        {
            return Reflection::Create(ReflectedObject(val.Get<shared_ptr<FormulaDataMap>>().get()));
        }
        else if (val.CanGet<shared_ptr<FormulaDataVector>>())
        {
            return Reflection::Create(ReflectedObject(val.Get<shared_ptr<FormulaDataVector>>().get()));
        }

        return Reflection::Create(&val);
    }
};

class FormulaDataVectorStructureWrapper : public FormulaDataStructureWrapper
{
public:
    FormulaDataVectorStructureWrapper()
    {
        caps.hasRangeAccess = true;
    }

    bool HasFields(const ReflectedObject& object, const ValueWrapper* vw) const override
    {
        FormulaDataVector* data = vw->GetValueObject(object).GetPtr<FormulaDataVector>();
        return !data->IsEmpty();
    }

    Reflection GetField(const ReflectedObject& obj, const ValueWrapper* vw, const Any& key) const override
    {
        FormulaDataVector* data = vw->GetValueObject(obj).GetPtr<FormulaDataVector>();
        if (key.CanCast<size_t>())
        {
            size_t index = key.Cast<size_t>();
            if (index < data->GetCount())
            {
                return CreateReflection(data->Get(index));
            }
        }
        return Reflection();
    }

    Vector<Reflection::Field> GetFields(const ReflectedObject& obj, const ValueWrapper* vw) const override
    {
        Vector<Reflection::Field> fields;
        FormulaDataVector* data = vw->GetValueObject(obj).GetPtr<FormulaDataVector>();

        for (size_t index = 0; index < data->GetCount(); index++)
        {
            fields.emplace_back(index, CreateReflection(data->Get(index)), nullptr);
        }

        return fields;
    }
};

class FormulaDataMapStructureWrapper : public FormulaDataStructureWrapper
{
public:
    FormulaDataMapStructureWrapper()
    {
        caps.canAddField = false;
        caps.canRemoveField = false;
        caps.hasFlatStruct = false;
        caps.hasDynamicStruct = true;
        caps.flatKeysType = Type::Instance<String>();
        caps.flatValuesType = Type::Instance<Any>();
    }

    bool HasFields(const ReflectedObject& object, const ValueWrapper* vw) const override
    {
        FormulaDataMap* data = vw->GetValueObject(object).GetPtr<FormulaDataMap>();
        return !data->IsEmpty();
    }

    Reflection GetField(const ReflectedObject& obj, const ValueWrapper* vw, const Any& key) const override
    {
        FormulaDataMap* data = vw->GetValueObject(obj).GetPtr<FormulaDataMap>();
        return CreateReflection(data->Find(key.Cast<String>()));
    }

    Vector<Reflection::Field> GetFields(const ReflectedObject& obj, const ValueWrapper* vw) const override
    {
        Vector<Reflection::Field> fields;
        FormulaDataMap* data = vw->GetValueObject(obj).GetPtr<FormulaDataMap>();

        for (const String& key : data->GetOrderedKeys())
        {
            fields.emplace_back(key, CreateReflection(data->Find(key)), nullptr);
        }

        return fields;
    }
};

DAVA_VIRTUAL_REFLECTION_IMPL(FormulaDataVector)
{
    ReflectionRegistrator<FormulaDataVector>::Begin(std::make_unique<FormulaDataVectorStructureWrapper>())
    .End();
}

DAVA_VIRTUAL_REFLECTION_IMPL(FormulaDataMap)
{
    ReflectionRegistrator<FormulaDataMap>::Begin(std::make_unique<FormulaDataMapStructureWrapper>())
    .End();
}
}
