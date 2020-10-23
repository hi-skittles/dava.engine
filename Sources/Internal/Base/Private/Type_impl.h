#pragma once

#ifndef __Dava_Type__
#include "Base/Type.h"
#endif

#include "Base/Vector.h"

namespace DAVA
{
namespace TypeDetail
{
template <typename T>
struct TypeSize
{
    static const size_t size = sizeof(T);
};

template <>
struct TypeSize<void>
{
    static const size_t size = 0;
};

template <>
struct TypeSize<const void>
{
    static const size_t size = 0;
};

template <typename T>
struct TypeHolder
{
    static Type* t;
    static Type* Instance()
    {
        return t;
    }
    static Type** InstancePointer()
    {
        return &t;
    }
};

template <typename T>
Type* TypeHolder<T>::t = Type::Init<T>();

static Type* nullType = nullptr;

template <typename T>
Type* const* GetTypeIfTrue(std::false_type)
{
    return &nullType;
}

template <typename T>
Type* const* GetTypeIfTrue(std::true_type)
{
    return TypeHolder<T>::InstancePointer();
}

template <typename T>
Type::SeedCastOP GetCastIfSeed(std::true_type)
{
    static auto op = [](const void* ptr) -> const Type::Seed*
    {
        const T* tptr = static_cast<const T*>(ptr);
        return static_cast<const Type::Seed*>(tptr);
    };
    return static_cast<Type::SeedCastOP>(op);
}

template <typename T>
Type::SeedCastOP GetCastIfSeed(std::false_type)
{
    return nullptr;
}

} // namespace TypeDetails

struct Type::Seed
{
};

class TypeDB
{
public:
    struct Stats
    {
        size_t typesCount = 0;
        size_t typesMemory = 0;
        size_t typeInheritanceCount = 0;
        size_t typeInheritanceInfoCount = 0;
        size_t typeInheritanceMemory = 0;
        size_t typeDBMemory = 0;
        size_t totalMemory = 0;
    };

    static TypeDB* GetLocalDB();
    static Stats GetLocalDBStats();

    void AddType(Type** type);
    void SetMasterDB(TypeDB* masterDB);

private:
    static TypeDB** GetLocalDBPtr();

    Vector<Type**> types;
};

inline uint32_t Type::GetSize() const
{
    return size;
}

inline const char* Type::GetName() const
{
    return stdTypeInfo->name();
}

inline std::type_index Type::GetTypeIndex() const
{
    return std::type_index(*stdTypeInfo);
}

inline const TypeInheritance* Type::GetInheritance() const
{
    return inheritance.get();
}

inline unsigned long Type::GetTypeFlags() const
{
    return flags.to_ulong();
}

template <typename T>
inline bool Type::Is() const
{
    return (this == Instance<T>());
}

inline bool Type::IsConst() const
{
    return flags.test(static_cast<size_t>(eTypeFlag::isConst));
}

inline bool Type::IsPointer() const
{
    return flags.test(static_cast<size_t>(eTypeFlag::isPointer));
}

inline bool Type::IsReference() const
{
    return flags.test(static_cast<size_t>(eTypeFlag::isReference));
}

inline bool Type::IsFundamental() const
{
    return flags.test(static_cast<size_t>(eTypeFlag::isFundamental));
}

inline bool Type::IsTrivial() const
{
    return flags.test(static_cast<size_t>(eTypeFlag::isTrivial));
}

inline bool Type::IsIntegral() const
{
    return flags.test(static_cast<size_t>(eTypeFlag::isIntegral));
}

inline bool Type::IsFloatingPoint() const
{
    return flags.test(static_cast<size_t>(eTypeFlag::isFloatingPoint));
}

inline bool Type::IsEnum() const
{
    return flags.test(static_cast<size_t>(eTypeFlag::isEnum));
}

inline bool Type::IsAbstract() const
{
    return flags.test(static_cast<size_t>(eTypeFlag::isAbstract));
}

inline Type::SeedCastOP Type::GetSeedCastOP() const
{
    return seedCastOP;
}

inline const Type* Type::Decay() const
{
    const Type* decayedType = *decayType;

    if (nullptr != decayedType)
        return decayedType;

    return this;
}

inline const Type* Type::Deref() const
{
    return *derefType;
}

inline const Type* Type::Pointer() const
{
    return *pointerType;
}

template <typename T>
Type* Type::Init()
{
    static Type type;

    using DerefU = DerefT<T>;
    using DecayU = DecayT<T>;
    using PointerU = PointerT<T>;

    static const bool needDeref = (!std::is_same<T, DerefU>::value);
    static const bool needDecay = (!std::is_same<T, DecayU>::value);
    static const bool needPointer = (!std::is_pointer<T>::value);
    static const bool needSeed = (std::is_base_of<Type::Seed, T>::value || std::is_same<Type::Seed, T>::value);

    type.size = static_cast<uint32_t>(TypeDetail::TypeSize<T>::size);
    type.name = typeid(T).name();
    type.stdTypeInfo = &typeid(T);

    type.flags.set(isConst, std::is_const<T>::value);
    type.flags.set(isPointer, std::is_pointer<T>::value);
    type.flags.set(isPointerToConst, std::is_pointer<T>::value && std::is_const<typename std::remove_pointer<T>::type>::value);
    type.flags.set(isReference, std::is_reference<T>::value);
    type.flags.set(isReferenceToConst, std::is_reference<T>::value && std::is_const<typename std::remove_reference<T>::type>::value);
    type.flags.set(isFundamental, std::is_fundamental<T>::value);
    type.flags.set(isTrivial, std::is_trivial<T>::value);
    type.flags.set(isIntegral, std::is_integral<T>::value);
    type.flags.set(isFloatingPoint, std::is_floating_point<T>::value);
    type.flags.set(isEnum, std::is_enum<T>::value);
    type.flags.set(isAbstract, std::is_abstract<T>::value);

    auto condSeed = std::integral_constant<bool, needSeed>();
    type.seedCastOP = TypeDetail::GetCastIfSeed<T>(condSeed);

    auto condDeref = std::integral_constant<bool, needDeref>();
    type.derefType = TypeDetail::GetTypeIfTrue<DerefU>(condDeref);

    auto condDecay = std::integral_constant<bool, needDecay>();
    type.decayType = TypeDetail::GetTypeIfTrue<DecayU>(condDecay);

    auto condPointer = std::integral_constant<bool, needPointer>();
    type.pointerType = TypeDetail::GetTypeIfTrue<PointerU>(condPointer);

    TypeDB::GetLocalDB()->AddType(TypeDetail::TypeHolder<T>::InstancePointer());

    return &type;
}

template <typename T>
const Type* Type::Instance()
{
    return TypeDetail::TypeHolder<T>::Instance();
}

} // namespace DAVA
