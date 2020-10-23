#include "Reflection/ReflectedTypeDB.h"
#include "Reflection/Private/Wrappers/StructureWrapperClass.h"

namespace DAVA
{
StructureWrapperClass::StructureWrapperClass(const Type* type_)
    : rootType(ReflectedTypeDB::GetByType(type_))
{
    caps.hasRangeAccess = true;

    FillCache(rootType);
}

void StructureWrapperClass::Update()
{
    fieldsCache.clear();
    methodsCache.clear();

    FillCache(rootType);
}

void StructureWrapperClass::FillCache(const ReflectedType* reflectedType)
{
    FillCacheEntries(reflectedType);

    const TypeInheritance* inheritance = reflectedType->GetType()->GetInheritance();
    if (nullptr != inheritance)
    {
        const Vector<TypeInheritance::Info>& baseTypesInfo = inheritance->GetBaseTypes();
        for (auto& baseInfo : baseTypesInfo)
        {
            FillCache(ReflectedTypeDB::GetByType(baseInfo.type));
        }
    }
}

void StructureWrapperClass::FillCacheEntries(const ReflectedType* reflectedType)
{
    const ReflectedStructure* structure = reflectedType->GetStructure();
    if (nullptr != structure)
    {
        for (auto& f : structure->fields)
        {
            const ReflectedStructure::Field* field = f.get();

            fieldsCache.push_back({ field, reflectedType });
            fieldsNameIndexes[field->name] = fieldsCache.size() - 1;
        }

        for (auto& m : structure->methods)
        {
            const ReflectedStructure::Method* method = m.get();

            methodsCache.push_back({ method });
            methodsNameIndexes[method->name] = methodsCache.size() - 1;
        }
    }
}

bool StructureWrapperClass::HasFields(const ReflectedObject& object, const ValueWrapper* vw) const
{
    return !fieldsCache.empty();
}

size_t StructureWrapperClass::GetFieldsCount(const ReflectedObject& object, const ValueWrapper* vw) const
{
    return fieldsCache.size();
}

Reflection StructureWrapperClass::GetField(const ReflectedObject& object, const ValueWrapper* vw, const Any& key) const
{
    if (!fieldsCache.empty())
    {
        ReflectedStructure::Key name = key.Cast<ReflectedStructure::Key>(ReflectedStructure::Key());
        if (!name.empty())
        {
            auto it = fieldsNameIndexes.find(name);
            if (it != fieldsNameIndexes.end())
            {
                const ReflectedStructure::Field* field = fieldsCache[it->second].field;
                return Reflection(vw->GetValueObject(object), field->valueWrapper.get(), nullptr, field->meta.get());
            }
        }
    }

    return Reflection();
}

Vector<Reflection::Field> StructureWrapperClass::GetFields(const ReflectedObject& object, const ValueWrapper* vw) const
{
    Vector<Reflection::Field> ret;

    ret.reserve(fieldsCache.size());
    for (const CachedFieldEntry& fieldEntry : fieldsCache)
    {
        const ReflectedStructure::Field* field = fieldEntry.field;

        Any key(field->name);
        Reflection ref(vw->GetValueObject(object), field->valueWrapper.get(), nullptr, field->meta.get());

        ret.emplace_back(std::move(key), std::move(ref), fieldEntry.inheritFrom);
    }

    return ret;
}

Vector<Reflection::Field> StructureWrapperClass::GetFields(const ReflectedObject& object, const ValueWrapper* vw, size_t first, size_t count) const
{
    Vector<Reflection::Field> ret;

    size_t sz = fieldsCache.size();

    DVASSERT(first < sz);
    DVASSERT(first + count <= sz);

    if (first < sz)
    {
        size_t n = std::min(count, sz - first);

        ret.reserve(n);
        for (size_t i = first; i < (first + n); ++i)
        {
            const ReflectedStructure::Field* field = fieldsCache[i].field;

            Any key(field->name);
            Reflection ref(vw->GetValueObject(object), field->valueWrapper.get(), nullptr, field->meta.get());

            ret.emplace_back(std::move(key), std::move(ref), fieldsCache[i].inheritFrom);
        }
    }

    return ret;
}

bool StructureWrapperClass::HasMethods(const ReflectedObject& object, const ValueWrapper* vw) const
{
    return !methodsCache.empty();
}

AnyFn StructureWrapperClass::GetMethod(const ReflectedObject& object, const ValueWrapper* vw, const Any& key) const
{
    FastName name = key.Cast<FastName>(FastName());
    if (name.IsValid())
    {
        void* this_ = vw->GetValueObject(object).GetVoidPtr();

        ReflectedStructure::Key name = key.Cast<ReflectedStructure::Key>(ReflectedStructure::Key());
        if (!name.empty())
        {
            auto it = methodsNameIndexes.find(name);
            if (it != methodsNameIndexes.end())
            {
                return methodsCache[it->second].method->fn.BindThis(this_);
            }
        }
    }
    return AnyFn();
}

Vector<Reflection::Method> StructureWrapperClass::GetMethods(const ReflectedObject& object, const ValueWrapper* vw) const
{
    Vector<Reflection::Method> ret;

    void* this_ = vw->GetValueObject(object).GetVoidPtr();

    ret.reserve(methodsCache.size());
    for (const CachedMethodEntry& methodEntry : methodsCache)
    {
        const ReflectedStructure::Method* method = methodEntry.method;

        AnyFn fn = method->fn.BindThis(this_);
        ret.emplace_back(method->name, std::move(fn));
    }

    return ret;
}

} //namespace DAVA
