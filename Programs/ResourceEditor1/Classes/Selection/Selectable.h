#pragma once

#include "Scene3D/Entity.h"
#include <TArc/Utils/ReflectionHelpers.h>
#include <Reflection/ReflectedType.h>
#include <Base/TypeInheritance.h>
#include <Base/Type.h>
#include <type_traits>

class Selectable
{
public:
    enum class TransformPivot : DAVA::uint32
    {
        ObjectCenter,
        CommonCenter
    };

    enum class TransformType : DAVA::uint32
    {
        Disabled,
        Translation,
        Rotation,
        Scale
    };

    class TransformProxy
    {
    public:
        virtual ~TransformProxy() = default;
        virtual const DAVA::Matrix4& GetWorldTransform(const DAVA::Any& object) = 0;
        virtual const DAVA::Matrix4& GetLocalTransform(const DAVA::Any& object) = 0;
        virtual void SetLocalTransform(DAVA::Any& object, const DAVA::Matrix4& matrix) = 0;
        virtual bool SupportsTransformType(const DAVA::Any& object, TransformType transformType) const = 0;
        virtual bool TransformDependsFromObject(const DAVA::Any& dependant, const DAVA::Any& dependsOn) const = 0;
    };

    template <typename CLASS, typename PROXY>
    static void AddTransformProxyForClass();
    static void RemoveAllTransformProxies();

public:
    Selectable() = default;
    explicit Selectable(const DAVA::Any& object);
    Selectable(const Selectable& other);
    Selectable(Selectable&& other);

    Selectable& operator=(const Selectable& other);
    Selectable& operator=(Selectable&& other);

    bool operator==(const Selectable& other) const;
    bool operator!=(const Selectable& other) const;

    // comparing only pointers here, and not using bounding box
    // added for compatibility with sorted containers
    bool operator<(const Selectable& other) const;

    const DAVA::ReflectedType* GetObjectType() const;

    template <typename T>
    bool CanBeCastedTo() const;

    template <typename T>
    T* Cast() const;

    const DAVA::Any& GetContainedObject() const;
    DAVA::Entity* AsEntity() const;

    const DAVA::AABBox3& GetBoundingBox() const;
    void SetBoundingBox(const DAVA::AABBox3& box);

    bool SupportsTransformType(TransformType) const;
    const DAVA::Matrix4& GetLocalTransform() const;
    const DAVA::Matrix4& GetWorldTransform() const;
    void SetLocalTransform(const DAVA::Matrix4& transform);

    bool TransformDependsOn(const Selectable&) const;

    bool ContainsObject() const;

private:
    static void AddConcreteProxy(const DAVA::Type* classInfo, TransformProxy* proxy);
    static TransformProxy* GetTransformProxyForClass(const DAVA::Any& object);

private:
    DAVA::Any object;
    DAVA::AABBox3 boundingBox;
};

template <typename T>
bool Selectable::CanBeCastedTo() const
{
    if (ContainsObject() == false)
    {
        return false;
    }
    DVASSERT(object.GetType()->IsPointer());
    const DAVA::ReflectedType* t = DAVA::TArc::GetValueReflectedType(object);
    DVASSERT(t != nullptr);
    return DAVA::TypeInheritance::CanCast(t->GetType()->Pointer(), DAVA::Type::Instance<T*>());
}

template <typename T>
inline T* Selectable::Cast() const
{
    DVASSERT(ContainsObject() == true);
    DVASSERT(object.GetType()->IsPointer());
    const DAVA::ReflectedType* t = DAVA::TArc::GetValueReflectedType(object);
    DVASSERT(t != nullptr);
    const DAVA::Type* objType = t->GetType()->Pointer();
    const DAVA::Type* castToType = DAVA::Type::Instance<T*>();
    if (DAVA::TypeInheritance::CanCast(objType, castToType) == false)
    {
        DVASSERT(false);
        return nullptr;
    }

    void* inPtr = object.Get<void*>();
    void* outPtr = nullptr;
    bool casted = DAVA::TypeInheritance::Cast(objType, castToType, inPtr, &outPtr);
    DVASSERT(casted == true);
    return reinterpret_cast<T*>(outPtr);
}

inline const DAVA::Any& Selectable::GetContainedObject() const
{
    return object;
}

inline const DAVA::AABBox3& Selectable::GetBoundingBox() const
{
    return boundingBox;
}

inline DAVA::Entity* Selectable::AsEntity() const
{
    if (CanBeCastedTo<DAVA::Entity>() == false)
    {
        return nullptr;
    }
    return Cast<DAVA::Entity>();
}

template <typename CLASS, typename PROXY>
inline void Selectable::AddTransformProxyForClass()
{
    static_assert(std::is_base_of<Selectable::TransformProxy, PROXY>::value,
                  "Transform proxy should be derived from Selectable::TransformProxy");
    AddConcreteProxy(DAVA::Type::Instance<CLASS>(), new PROXY());
}

inline bool Selectable::ContainsObject() const
{
    return object.IsEmpty() == false;
}

namespace std
{
template <>
struct hash<Selectable>
{
    size_t operator()(const Selectable& object) const
    {
        const DAVA::Any& obj = object.GetContainedObject();
        DVASSERT(obj.GetType()->IsPointer() == true);
        return reinterpret_cast<size_t>(obj.Get<void*>());
    }
};
}
