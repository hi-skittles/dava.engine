#pragma once

#include "FileSystem/VariantType.h"
#include "Reflection/Private/Wrappers/StructureWrapperDefault.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
VariantType PrepareValueForKeyedArchiveImpl(const Any& value, VariantType::eVariantType resultType);

class KeyedArchiveElementValueWrapper : public ValueWrapper
{
public:
    const Type* GetType(const ReflectedObject& object) const override;
    bool IsReadonly(const ReflectedObject& object) const override;
    Any GetValue(const ReflectedObject& object) const override;
    bool SetValue(const ReflectedObject& object, const Any& value) const override;
    bool SetValueWithCast(const ReflectedObject& object, const Any& value) const override;
    ReflectedObject GetValueObject(const ReflectedObject& object) const override;
};

class KeyedArchiveStructureWrapper : public StructureWrapperDefault
{
public:
    KeyedArchiveStructureWrapper();

    Reflection CreateReflection(VariantType* v, bool isArchiveConst) const;

    bool HasFields(const ReflectedObject& object, const ValueWrapper* vw) const override;
    Reflection GetField(const ReflectedObject& object, const ValueWrapper* vw, const Any& key) const override;
    Vector<Reflection::Field> GetFields(const ReflectedObject& object, const ValueWrapper* vw) const override;

    bool AddField(const ReflectedObject& object, const ValueWrapper* vw, const Any& key, const Any& value) const override;
    bool RemoveField(const ReflectedObject& object, const ValueWrapper* vw, const Any& key) const override;

private:
    KeyedArchiveElementValueWrapper valueWrapper;
};
} // namespace DAVA