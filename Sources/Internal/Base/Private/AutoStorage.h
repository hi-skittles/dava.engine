#pragma once
#define DAVAENGINE_AUTO_STORAGE__H

#include <memory>
#include <type_traits>
#include "Base/Array.h"
#include "Debug/DVAssert.h"

namespace DAVA
{
struct AutoStorageCountTraits
{
    enum : size_t
    {
        SharedSize = sizeof(std::shared_ptr<void>),
        VoidSize = sizeof(void*),
        Count = static_cast<size_t>((SharedSize + VoidSize - 1) / static_cast<double>(VoidSize))
    };
};

template <size_t Count = AutoStorageCountTraits::Count>
class AutoStorage final
{
    using StorageT = Array<void*, Count>;
    using SharedT = std::shared_ptr<void>;

    static_assert(Count > 0, "Size should be > 0");
    static_assert(std::tuple_size<StorageT>::value >= AutoStorageCountTraits::Count &&
                  std::tuple_size<StorageT>::value * sizeof(void*) >= sizeof(SharedT),
                  "Wrong Autostorage size");

public:
    template <typename T>
    using StorableType = typename std::decay<T>::type;

    AutoStorage();
    ~AutoStorage();

    AutoStorage(AutoStorage&&);
    AutoStorage(const AutoStorage&);

    AutoStorage& operator=(const AutoStorage& value);
    AutoStorage& operator=(AutoStorage&& value);

    bool operator==(const AutoStorage&) const = delete;
    bool operator!=(const AutoStorage&) const = delete;

    bool IsEmpty() const;
    bool IsSimple() const;

    void Clear();
    void Swap(AutoStorage& value);

    template <typename T>
    void SetSimple(T&& value);

    template <typename T>
    void SetShared(T&& value);

    template <typename T>
    void SetAuto(T&& value);

    void SetData(const void*, size_t size);

    template <typename T>
    const T& GetSimple() const;

    template <typename T>
    const T& GetShared() const;

    template <typename T>
    const T& GetAuto() const;

    const void* GetData() const;

    template <typename T>
    struct IsSimpleType
    {
        static const bool value =
        (sizeof(T) <= sizeof(StorageT))
        && std::is_trivially_copy_constructible<T>::value
        && std::is_trivially_copy_assignable<T>::value
        && std::is_trivially_destructible<T>::value;
    };

private:
    enum class StorageType
    {
        Empty,
        Simple,
        Shared
    };

    StorageT storage;
    StorageType type = StorageType::Empty;

    SharedT* SharedPtr() const;

    void DoCopy(const AutoStorage& value);
    void DoMove(AutoStorage&& value);

    template <typename T>
    void SetAutoImpl(T&& value, std::true_type);

    template <typename T>
    void SetAutoImpl(T&& value, std::false_type);

    template <typename T>
    const T& GetAutoImpl(std::true_type) const;

    template <typename T>
    const T& GetAutoImpl(std::false_type) const;
};

} // namespace DAVA

#define __DAVA_AutoStorage__
#include "Base/Private/AutoStorage_impl.h"
