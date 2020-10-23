#include "Reflection/ReflectedTypeDB.h"

namespace DAVA
{
ReflectedType::~ReflectedType() = default;

ReflectedType::ReflectedType(const Type* type_)
    : type(type_)
{
}

Vector<const AnyFn*> ReflectedType::GetCtors() const
{
    Vector<const AnyFn*> ret;

    ret.reserve(structure->ctors.size());
    for (auto& it : structure->ctors)
    {
        ret.push_back(it.get());
    }

    return ret;
}

const AnyFn* ReflectedType::GetDtor() const
{
    if (structure->dtor != nullptr)
    {
        return structure->dtor.get();
    }

    return nullptr;
}

void ReflectedType::Destroy(Any&& v) const
{
    if (v.GetType()->IsPointer())
    {
        const AnyFn* dtor = GetDtor();

        if (nullptr != dtor)
        {
            dtor->InvokeWithCast(v);
        }
        else
        {
            DAVA_THROW(Exception, "There is no appropriate dtor.");
        }
    }

    v.Clear();
}
} // namespace DAVA
