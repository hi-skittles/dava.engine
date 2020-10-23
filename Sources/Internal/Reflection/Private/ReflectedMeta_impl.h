#pragma once

#ifndef __DAVA_Reflection__
#include "Reflection/Reflection.h"
#endif

namespace DAVA
{
template <typename T, typename IndexT>
template <typename... Args>
Meta<T, IndexT>::Meta(Args&&... args)
    : T(std::forward<Args>(args)...)
{
    static_assert(std::is_same<IndexT, T>::value || std::is_base_of<IndexT, T>::value, "T should be derived from IndexT or the same as IndexT");
}

inline ReflectedMeta::ReflectedMeta(ReflectedMeta&& rm)
    : metas(std::move(rm.metas))
{
}

template <typename T, typename IndexT>
inline ReflectedMeta::ReflectedMeta(Meta<T, IndexT>&& meta)
{
    Emplace(std::move(meta));
}

template <typename T>
const T* ReflectedMeta::GetMeta() const
{
    return static_cast<const T*>(GetMeta(Type::Instance<T>()));
}

inline const void* ReflectedMeta::GetMeta(const Type* metaType) const
{
    const void* meta = nullptr;

    auto it = metas.find(metaType);
    if (it != metas.end())
    {
        // Here we know, that requested type T == Meta<IndexT>, in other situation we will fail on search
        // As we store value in metas as Any(Meta<T, IndexT>) and we know that Meta derived from T and T derived
        // from IndexT or same as T we can get raw pointer from Any
        meta = it->second.GetData();
    }

    return meta;
}

template <typename T, typename IndexT>
void ReflectedMeta::Emplace(Meta<T, IndexT>&& meta)
{
    metas.emplace(Type::Instance<Meta<IndexT>>(), std::move(meta));
}

template <typename T, typename IndexT, typename U, typename IndexU>
inline ReflectedMeta operator, (Meta<T, IndexT> && metaa, Meta<U, IndexU>&& metab)
{
    ReflectedMeta ret;

    ret.Emplace(std::move(metaa));
    ret.Emplace(std::move(metab));

    return ret;
}

template <typename T, typename IndexT>
ReflectedMeta&& operator, (ReflectedMeta && rmeta, Meta<T, IndexT>&& meta)
{
    rmeta.Emplace(std::move(meta));
    return std::move(rmeta);
}

} // namespace DAVA
