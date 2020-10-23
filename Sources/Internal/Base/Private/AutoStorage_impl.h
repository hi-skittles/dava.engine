#pragma once

#ifndef __DAVA_AutoStorage__
#include "Base/Private/AutoStorage.h"
#endif

namespace DAVA
{
template <size_t Count>
inline AutoStorage<Count>::AutoStorage()
{
    // storage.fill(nullptr);
}

template <size_t Count>
inline AutoStorage<Count>::~AutoStorage()
{
    Clear();
}

template <size_t Count>
inline AutoStorage<Count>::AutoStorage(AutoStorage&& autoStorage)
{
    DoMove(std::move(autoStorage));
}

template <size_t Count>
inline AutoStorage<Count>::AutoStorage(const AutoStorage& autoStorage)
{
    DoCopy(autoStorage);
}

template <size_t Count>
inline AutoStorage<Count>& AutoStorage<Count>::operator=(const AutoStorage& value)
{
    if (this != &value)
    {
        Clear();
        DoCopy(value);
    }

    return *this;
}

template <size_t Count>
inline AutoStorage<Count>& AutoStorage<Count>::operator=(AutoStorage&& value)
{
    if (this != &value)
    {
        Clear();
        DoMove(std::move(value));
    }

    return *this;
}

template <size_t Count>
inline bool AutoStorage<Count>::IsEmpty() const
{
    return (StorageType::Empty == type);
}

template <size_t Count>
inline bool AutoStorage<Count>::IsSimple() const
{
    return (StorageType::Simple == type);
}

template <size_t Count>
inline void AutoStorage<Count>::Clear()
{
    if (StorageType::Shared == type)
    {
        SharedPtr()->reset();
    }

    type = StorageType::Empty;
}

template <size_t Count>
inline void AutoStorage<Count>::Swap(AutoStorage& autoStorage)
{
    AutoStorage tmp(std::move(autoStorage));
    autoStorage = std::move(*this);
    *this = std::move(tmp);
}

template <size_t Count>
template <typename T>
inline void AutoStorage<Count>::SetSimple(T&& value)
{
    using U = StorableType<T>;

    static_assert(IsSimpleType<U>::value, "Type should be simple");

    Clear();

    type = StorageType::Simple;
    new (storage.data()) U(std::forward<T>(value));
}

template <size_t Count>
template <typename T>
void AutoStorage<Count>::SetShared(T&& value)
{
    using U = StorableType<T>;

    Clear();
    type = StorageType::Shared;
    U* valueCopy = new U(std::forward<T>(value));
    new (storage.data()) SharedT(valueCopy);
}

template <size_t Count>
template <typename T>
inline void AutoStorage<Count>::SetAuto(T&& value)
{
    using U = StorableType<T>;

    auto tp = std::integral_constant<bool, IsSimpleType<U>::value>();
    SetAutoImpl(std::forward<T>(value), tp);
}

template <size_t Count>
inline void AutoStorage<Count>::SetData(const void* data, size_t size)
{
    Clear();

    if (size <= sizeof(StorageT))
    {
        type = AutoStorage::StorageType::Simple;
        std::memcpy(storage.data(), data, size);
    }
    else
    {
        type = AutoStorage::StorageType::Shared;
        char* arr = new char[size];
        std::memcpy(arr, data, size);
        new (storage.data()) SharedT(arr, [](char* p) { delete[] p; });
    }
}

template <size_t Count>
template <typename T>
inline const T& AutoStorage<Count>::GetSimple() const
{
    using U = StorableType<T>;

    DVASSERT(StorageType::Simple == type);
    return *(reinterpret_cast<const U*>(const_cast<void* const*>(storage.data())));
}

template <size_t Count>
template <typename T>
const T& AutoStorage<Count>::GetShared() const
{
    using U = StorableType<T>;

    DVASSERT(StorageType::Shared == type);
    return *(static_cast<const U*>(SharedPtr()->get()));
}

template <size_t Count>
template <typename T>
inline const T& AutoStorage<Count>::GetAuto() const
{
    using U = StorableType<T>;

    DVASSERT(StorageType::Empty != type);

    auto tp = std::integral_constant<bool, IsSimpleType<U>::value>();
    return GetAutoImpl<U>(tp);
}

template <size_t Count>
inline const void* AutoStorage<Count>::GetData() const
{
    DVASSERT(StorageType::Empty != type);

    return (StorageType::Simple == type) ? storage.data() : static_cast<const void*>(SharedPtr()->get());
}

template <size_t Count>
inline void AutoStorage<Count>::DoCopy(const AutoStorage& value)
{
    type = value.type;

    if (StorageType::Shared == type)
    {
        new (storage.data()) SharedT(*value.SharedPtr());
    }
    else
    {
        storage = value.storage;
    }
}

template <size_t Count>
inline void AutoStorage<Count>::DoMove(AutoStorage&& value)
{
    type = value.type;
    storage = std::move(value.storage);
    value.type = StorageType::Empty;
}

template <size_t Count>
inline typename AutoStorage<Count>::SharedT* AutoStorage<Count>::SharedPtr() const
{
    return reinterpret_cast<SharedT*>(const_cast<void**>(storage.data()));
}

template <size_t Count>
template <typename T>
inline void AutoStorage<Count>::SetAutoImpl(T&& value, std::true_type)
{
    SetSimple(std::forward<T>(value));
}

template <size_t Count>
template <typename T>
inline void AutoStorage<Count>::SetAutoImpl(T&& value, std::false_type)
{
    SetShared(std::forward<T>(value));
}

template <size_t Count>
template <typename T>
inline const T& AutoStorage<Count>::GetAutoImpl(std::true_type) const
{
    return GetSimple<T>();
}

template <size_t Count>
template <typename T>
inline const T& AutoStorage<Count>::GetAutoImpl(std::false_type) const
{
    return GetShared<T>();
}

} // namespace DAVA
