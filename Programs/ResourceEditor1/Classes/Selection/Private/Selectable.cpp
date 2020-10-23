#include "Classes/Selection/Selectable.h"

#include <Base/Type.h>
#include <Reflection/ReflectedTypeDB.h>

Selectable::Selectable(const DAVA::Any& object_)
    : object(object_)
{
    DVASSERT(ContainsObject() == true);
}

Selectable::Selectable(const Selectable& other)
    : object(other.object)
    , boundingBox(other.boundingBox)
{
}

Selectable::Selectable(Selectable&& other)
    : boundingBox(other.boundingBox)
{
    std::swap(object, other.object);
}

Selectable& Selectable::operator=(const Selectable& other)
{
    if (this != &other)
    {
        object = other.object;
        boundingBox = other.boundingBox;
    }
    return *this;
}

Selectable& Selectable::operator=(Selectable&& other)
{
    if (this != &other)
    {
        object = other.object;
        boundingBox = other.boundingBox;
        other.object = DAVA::Reflection();
    }
    return *this;
}

bool Selectable::operator==(const Selectable& other) const
{
    return object == other.object;
}

bool Selectable::operator!=(const Selectable& other) const
{
    return object != other.object;
}

bool Selectable::operator<(const Selectable& other) const
{
    return DAVA::PointerValueAnyLess()(object, other.object);
}

const DAVA::ReflectedType* Selectable::GetObjectType() const
{
    if (ContainsObject() == false)
    {
        return nullptr;
    }

    DVASSERT(object.GetType()->IsPointer());
    return DAVA::TArc::GetValueReflectedType(object);
}

void Selectable::SetBoundingBox(const DAVA::AABBox3& box)
{
    boundingBox = box;
}

const DAVA::Matrix4& Selectable::GetLocalTransform() const
{
    auto proxy = GetTransformProxyForClass(object);
    return (proxy == nullptr) ? DAVA::Matrix4::IDENTITY : proxy->GetLocalTransform(object);
}

const DAVA::Matrix4& Selectable::GetWorldTransform() const
{
    auto proxy = GetTransformProxyForClass(object);
    return (proxy == nullptr) ? DAVA::Matrix4::IDENTITY : proxy->GetWorldTransform(object);
}

void Selectable::SetLocalTransform(const DAVA::Matrix4& transform)
{
    auto proxy = GetTransformProxyForClass(object);
    if (proxy != nullptr)
    {
        proxy->SetLocalTransform(object, transform);
    }
}

bool Selectable::SupportsTransformType(TransformType transformType) const
{
    auto proxyClass = GetTransformProxyForClass(object);
    return (proxyClass != nullptr) && proxyClass->SupportsTransformType(object, transformType);
}

bool Selectable::TransformDependsOn(const Selectable& other) const
{
    auto proxyClass = GetTransformProxyForClass(object);
    return (proxyClass != nullptr) && proxyClass->TransformDependsFromObject(object, other.GetContainedObject());
}

/*
 * Transform proxy stuff
 */
static DAVA::Map<const DAVA::Type*, Selectable::TransformProxy*> transformProxies;

void Selectable::AddConcreteProxy(const DAVA::Type* classInfo, Selectable::TransformProxy* proxy)
{
    DVASSERT(transformProxies.count(classInfo) == 0);
    transformProxies.emplace(classInfo, proxy);
}

Selectable::TransformProxy* Selectable::GetTransformProxyForClass(const DAVA::Any& object)
{
    const DAVA::Type* t = object.GetType();
    DVASSERT(t->IsPointer() == true);
    const DAVA::ReflectedType* refType = DAVA::ReflectedTypeDB::GetByPointer(object.Get<void*>(), t->Deref());
    auto i = transformProxies.find(refType->GetType());
    return (i == transformProxies.end()) ? nullptr : i->second;
}

void Selectable::RemoveAllTransformProxies()
{
    for (auto& tp : transformProxies)
    {
        delete tp.second;
    }
    transformProxies.clear();
}
