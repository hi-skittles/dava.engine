#pragma once

#ifndef __DAVA_Reflection__
#include "Reflection/Reflection.h"
#endif

#include "Reflection/Private/Wrappers/StructureWrapperDefault.h"
#include "Reflection/Private/Wrappers/ValueWrapperDefault.h"

namespace DAVA
{
template <typename C>
class StructureWrapperStdSet : public StructureWrapperDefault
{
public:
    using K = typename C::key_type;

    StructureWrapperStdSet()
    {
        caps.canAddField = true;
        caps.canRemoveField = true;
        caps.hasFlatStruct = true;
        caps.hasDynamicStruct = true;
        caps.flatKeysType = Type::Instance<K>();
        caps.flatValuesType = Type::Instance<K>();
    }

    bool HasFields(const ReflectedObject& object, const ValueWrapper* vw) const override
    {
        C* c = vw->GetValueObject(object).GetPtr<C>();
        return (c->size() > 0);
    }

    Reflection GetField(const ReflectedObject& obj, const ValueWrapper* vw, const Any& key) const override
    {
        if (key.CanCast<K>())
        {
            const K& k = key.Cast<K>();
            C* c = vw->GetValueObject(obj).GetPtr<C>();

            auto it = c->find(k);
            if (it != c->end())
            {
                // std::set values shouldn't be modified
                // so get const pointer on value
                const K* v = &(*it);
                return Reflection::Create(v);
            }
        }

        return Reflection();
    }

    Vector<Reflection::Field> GetFields(const ReflectedObject& obj, const ValueWrapper* vw) const override
    {
        Vector<Reflection::Field> ret;

        C* c = vw->GetValueObject(obj).GetPtr<C>();

        ret.reserve(c->size());

        auto end = c->end();
        for (auto it = c->begin(); it != end; it++)
        {
            // std::set values shouldn't be modified
            // so get const pointer on value
            const K* v = &(*it);
            ret.emplace_back(Any(*v), Reflection::Create(v), nullptr);
        }

        return ret;
    }

    bool AddField(const ReflectedObject& object, const ValueWrapper* vw, const Any& key, const Any& value) const override
    {
        if (vw->IsReadonly(object))
        {
            return false;
        }

        C* c = vw->GetValueObject(object).GetPtr<C>();
        return c->insert(value.Get<K>()).second;
    }

    bool RemoveField(const ReflectedObject& object, const ValueWrapper* vw, const Any& key) const override
    {
        if (vw->IsReadonly(object))
        {
            return false;
        }

        if (!key.CanCast<K>())
        {
            return false;
        }

        K k = key.Cast<K>();
        C* c = vw->GetValueObject(object).GetPtr<C>();

        return (c->erase(k) > 0);
    }
};

template <typename T, typename Eq>
struct StructureWrapperCreator<Set<T, Eq>>
{
    static StructureWrapper* Create()
    {
        return new StructureWrapperStdSet<Set<T, Eq>>();
    }
};

template <typename T, typename Hash, typename Eq>
struct StructureWrapperCreator<UnorderedSet<T, Hash, Eq>>
{
    static StructureWrapper* Create()
    {
        return new StructureWrapperStdSet<UnorderedSet<T, Hash, Eq>>();
    }
};
} // namespace DAVA
