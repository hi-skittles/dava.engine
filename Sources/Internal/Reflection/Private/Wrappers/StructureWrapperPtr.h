#pragma once

#ifndef __DAVA_Reflection__
#include "Reflection/Reflection.h"
#endif

#include "Reflection/Private/Wrappers/StructureWrapperDefault.h"
#include "Reflection/Private/Wrappers/ValueWrapperDefault.h"

namespace DAVA
{
template <typename T>
class StructureWrapperPtr : public StructureWrapperDefault
{
public:
    static_assert(std::is_pointer<T>::value, "StructureWrapperPtr should be used as structure wrapper only for pointer fields.");

    StructureWrapperPtr() = default;

    bool HasFields(const ReflectedObject& obj, const ValueWrapper* vw) const override
    {
        ReflectedObject derefObj = Deref(vw->GetValueObject(obj));
        const StructureWrapper* sw = GetInternalWrapper(derefObj);
        if (nullptr != sw)
        {
            return sw->HasFields(derefObj, &ptrVW);
        }

        return StructureWrapperDefault::HasFields(obj, vw);
    }

    Reflection GetField(const ReflectedObject& obj, const ValueWrapper* vw, const Any& key) const override
    {
        ReflectedObject derefObj = Deref(vw->GetValueObject(obj));
        const StructureWrapper* sw = GetInternalWrapper(derefObj);
        if (nullptr != sw)
        {
            return sw->GetField(derefObj, &ptrVW, key);
        }

        return StructureWrapperDefault::GetField(obj, vw, key);
    }

    Vector<Reflection::Field> GetFields(const ReflectedObject& obj, const ValueWrapper* vw) const override
    {
        ReflectedObject derefObj = Deref(vw->GetValueObject(obj));
        const StructureWrapper* sw = GetInternalWrapper(derefObj);
        if (nullptr != sw)
        {
            return sw->GetFields(derefObj, &ptrVW);
        }

        return StructureWrapperDefault::GetFields(obj, vw);
    }

    bool HasMethods(const ReflectedObject& obj, const ValueWrapper* vw) const override
    {
        ReflectedObject derefObj = Deref(vw->GetValueObject(obj));
        const StructureWrapper* sw = GetInternalWrapper(derefObj);
        if (nullptr != sw)
        {
            return sw->HasMethods(derefObj, &ptrVW);
        }

        return StructureWrapperDefault::HasMethods(obj, vw);
    }

    AnyFn GetMethod(const ReflectedObject& obj, const ValueWrapper* vw, const Any& key) const override
    {
        ReflectedObject derefObj = Deref(vw->GetValueObject(obj));
        const StructureWrapper* sw = GetInternalWrapper(derefObj);
        if (nullptr != sw)
        {
            return sw->GetMethod(derefObj, &ptrVW, key);
        }

        return StructureWrapperDefault::GetMethod(obj, vw, key);
    }

    Vector<Reflection::Method> GetMethods(const ReflectedObject& obj, const ValueWrapper* vw) const override
    {
        ReflectedObject derefObj = Deref(vw->GetValueObject(obj));
        const StructureWrapper* sw = GetInternalWrapper(derefObj);
        if (nullptr != sw)
        {
            return sw->GetMethods(derefObj, &ptrVW);
        }

        return StructureWrapperDefault::GetMethods(obj, vw);
    }

    const Reflection::FieldCaps& GetFieldsCaps(const ReflectedObject& obj, const ValueWrapper* vw) const override
    {
        ReflectedObject derefObj = Deref(vw->GetValueObject(obj));
        const StructureWrapper* sw = GetInternalWrapper(derefObj);
        if (nullptr != sw)
        {
            return sw->GetFieldsCaps(derefObj, &ptrVW);
        }

        return StructureWrapperDefault::GetFieldsCaps(obj, vw);
    }

    AnyFn GetFieldCreator(const ReflectedObject& obj, const ValueWrapper* vw) const override
    {
        ReflectedObject derefObj = Deref(vw->GetValueObject(obj));
        const StructureWrapper* sw = GetInternalWrapper(derefObj);
        if (nullptr != sw)
        {
            return sw->GetFieldCreator(derefObj, &ptrVW);
        }

        return StructureWrapperDefault::GetFieldCreator(obj, vw);
    }

    bool AddField(const ReflectedObject& obj, const ValueWrapper* vw, const Any& key, const Any& value) const override
    {
        ReflectedObject derefObj = Deref(vw->GetValueObject(obj));
        const StructureWrapper* sw = GetInternalWrapper(derefObj);
        if (nullptr != sw)
        {
            return sw->AddField(derefObj, &ptrVW, key, value);
        }

        return StructureWrapperDefault::AddField(obj, vw, key, value);
    }

    bool InsertField(const ReflectedObject& obj, const ValueWrapper* vw, const Any& beforeKey, const Any& key, const Any& value) const override
    {
        ReflectedObject derefObj = Deref(vw->GetValueObject(obj));
        const StructureWrapper* sw = GetInternalWrapper(derefObj);
        if (nullptr != sw)
        {
            return sw->InsertField(obj, &ptrVW, beforeKey, key, value);
        }

        return StructureWrapperDefault::InsertField(obj, vw, beforeKey, key, value);
    }

    bool RemoveField(const ReflectedObject& obj, const ValueWrapper* vw, const Any& key) const override
    {
        ReflectedObject derefObj = Deref(vw->GetValueObject(obj));
        const StructureWrapper* sw = GetInternalWrapper(derefObj);
        if (nullptr != sw)
        {
            return sw->RemoveField(derefObj, &ptrVW, key);
        }

        return StructureWrapperDefault::RemoveField(obj, vw, key);
    }

protected:
    ValueWrapperDefault<T> ptrVW;

    virtual ReflectedObject Deref(const ReflectedObject& obj) const
    {
        if (obj.IsValid())
        {
            T* ptr = obj.GetPtr<T>();
            return ReflectedObject(*ptr);
        }

        return ReflectedObject();
    }

    virtual const StructureWrapper* GetInternalWrapper(ReflectedObject& derefObj) const
    {
        const StructureWrapper* sw = nullptr;

        if (derefObj.IsValid())
        {
            return derefObj.GetReflectedType()->GetStrucutreWrapper();
        }

        return sw;
    }
};

template <typename T>
class StructureWrapperSharedPtr final : public StructureWrapperPtr<T*>
{
protected:
    ReflectedObject Deref(const ReflectedObject& obj) const override
    {
        if (obj.IsValid())
        {
            std::shared_ptr<T>* sharedPtr = obj.GetPtr<std::shared_ptr<T>>();
            return ReflectedObject(sharedPtr->get());
        }

        return ReflectedObject();
    }
};

template <typename T>
struct StructureWrapperCreator<T*>
{
    static StructureWrapper* Create()
    {
        return new StructureWrapperPtr<T*>();
    }
};

template <typename T>
struct StructureWrapperCreator<std::shared_ptr<T>>
{
    static StructureWrapper* Create()
    {
        return new StructureWrapperSharedPtr<T>();
    }
};

} // namespace DAVA
