#pragma once
#ifndef __Dava_Any__
#include "Base/Any.h"
#endif
#include "Utils/StringFormat.h"

namespace DAVA
{
template <typename T>
bool AnyCompare<T>::IsEqual(const Any& v1, const Any&)
{
    String message = "Any::can't be equal compared for type %s";
    const Type* t = v1.GetType();
    if (t == nullptr)
    {
        message = Format(message.c_str(), "nullptr");
    }
    else
    {
        message = Format(message.c_str(), t->GetName());
    }
    DAVA_THROW(Exception, message);
    return false;
}

template <>
bool AnyCompare<String>::IsEqual(const Any& v1, const Any& v2);
extern template struct AnyCompare<String>;

template <>
bool AnyCompare<WideString>::IsEqual(const Any& v1, const Any& v2);
extern template struct AnyCompare<WideString>;

template <typename K, typename V, typename Eq>
struct AnyCompare<Map<K, V, Eq>>
{
    static bool IsEqual(const Any& v1, const Any& v2)
    {
        using MapType = Map<K, V, Eq>;
        const MapType& m1 = v1.Get<MapType>();
        const MapType& m2 = v2.Get<MapType>();
        return m1 == m2;
    }
};

template <typename K, typename V, typename Hash, typename Eq>
struct AnyCompare<UnorderedMap<K, V, Hash, Eq>>
{
    static bool IsEqual(const Any& v1, const Any& v2)
    {
        using MapType = UnorderedMap<K, V, Hash, Eq>;
        const MapType& m1 = v1.Get<MapType>();
        const MapType& m2 = v2.Get<MapType>();
        return m1 == m2;
    }
};

template <typename V, typename Eq>
struct AnyCompare<Set<V, Eq>>
{
    static bool IsEqual(const Any& v1, const Any& v2)
    {
        using SetType = Set<V, Eq>;
        const SetType& s1 = v1.Get<SetType>();
        const SetType& s2 = v2.Get<SetType>();
        return s1 == s2;
    }
};

template <typename V>
struct AnyCompare<Vector<V>>
{
    static bool IsEqual(const Any& v1, const Any& v2)
    {
        using VectorType = Vector<V>;
        const VectorType& vv1 = v1.Get<VectorType>();
        const VectorType& vv2 = v2.Get<VectorType>();
        return vv1 == vv2;
    }
};

} // namespace DAVA
