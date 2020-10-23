#pragma once

#ifndef __DAVA_Reflection__
#include "Reflection/Reflection.h"
#endif

namespace DAVA
{
class StructureWrapperDefault : public StructureWrapper
{
public:
    void Update() override
    {
    }

    bool HasFields(const ReflectedObject& object, const ValueWrapper* vw) const override
    {
        return false;
    }

    size_t GetFieldsCount(const ReflectedObject& object, const ValueWrapper* vw) const override
    {
        return 0;
    }

    Reflection GetField(const ReflectedObject& obj, const ValueWrapper* vw, const Any& key) const override
    {
        return Reflection();
    }

    Vector<Reflection::Field> GetFields(const ReflectedObject& obj, const ValueWrapper* vw) const override
    {
        return Vector<Reflection::Field>();
    }

    Vector<Reflection::Field> GetFields(const ReflectedObject& object, const ValueWrapper* vw, size_t first, size_t count) const override
    {
        return Vector<Reflection::Field>();
    }

    bool HasMethods(const ReflectedObject& object, const ValueWrapper* vw) const override
    {
        return false;
    }

    AnyFn GetMethod(const ReflectedObject& object, const ValueWrapper* vw, const Any& key) const override
    {
        return AnyFn();
    }

    Vector<Reflection::Method> GetMethods(const ReflectedObject& object, const ValueWrapper* vw) const override
    {
        return Vector<Reflection::Method>();
    }

    const Reflection::FieldCaps& GetFieldsCaps(const ReflectedObject& object, const ValueWrapper* vw) const override
    {
        return caps;
    }

    AnyFn GetFieldCreator(const ReflectedObject& object, const ValueWrapper* vw) const override
    {
        return AnyFn();
    }

    bool AddField(const ReflectedObject& object, const ValueWrapper* vw, const Any& key, const Any& value) const override
    {
        return false;
    }

    bool InsertField(const ReflectedObject& object, const ValueWrapper* vw, const Any& beforeKey, const Any& key, const Any& value) const override
    {
        return false;
    }

    bool RemoveField(const ReflectedObject& object, const ValueWrapper* vw, const Any& key) const override
    {
        return false;
    }

protected:
    Reflection::FieldCaps caps;
};

template <typename T>
struct StructureWrapperCreator
{
    static StructureWrapper* Create()
    {
        return new StructureWrapperDefault();
    }
};

} // namespace DAVA
