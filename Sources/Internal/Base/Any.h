#pragma once

#include "Base/Type.h"
#include "Base/TypeInheritance.h"
#include "Base/Exception.h"
#include "Base/Private/AutoStorage.h"

namespace DAVA
{
/** 
    \ingroup Base
    The class Any is a type-safe container for single value of any type.
    Stored value is always copied into internal storage. Implementations is encouraged to 
    avoid dynamic allocations for small objects, but such an optimization may only
    be applied to types for which std::is_nothrow_move_constructible returns true.
    \remark This class cannot be inherited.

    Any can be copied.
    - Internal storage with trivial value will also be copied.
    - Internal storage with complex type will be shared the same way as `std::shared_ptr` do.

    Typical usage:
    ```
    void foo()
    {
        int i = 1;
        const char* s = "Hello world";

        Any a;
        a.Set(i);
        std::count << a.Get<int>(); // prints "1"

        a.Set(s);
        std::count << a.Get<const char*>(); // prints "Hello world"


    }
    ```

    TODO: more description
    ...
*/
class Any final
{
public:
    using AnyStorage = AutoStorage<>;

    template <typename T>
    using NotAny = typename std::enable_if<!std::is_same<typename std::decay<T>::type, Any>::value, bool>::type;

    Any() = default;
    ~Any() = default;
    Any(const Any&) = default;

    Any(Any&& any);

    /** 
        Constructor. 
        \remark `notAny` is used to prevent creating Any from the other Any. Shouldn't be specified by user.
    */
    template <typename T>
    Any(T&& value, NotAny<T> notAny = true);

    /** Swaps this Any value with the given `any`. */
    void Swap(Any& any);

    /** Return `true` if Any is empty or `false` if isn't. */
    bool IsEmpty() const;

    /** Clears Any to its empty state. */
    void Clear();

    /** Gets the type of contained value. `null` will be returned if Any is empty. */
    const Type* GetType() const;

    /** Returns `true` if value with specified type T can get be from Any. */
    template <typename T>
    bool CanGet() const;

    /** 
        Gets value with specified type T from internal storage. 
        If specified T can't be get due to type mismatch `DAVA::Exception` will be raised. 
    */
    template <typename T>
    const T& Get() const;

    /** 
        Gets value with specified type T from internal storage.
        If specified T can't be get due to type mismatch then `defaultValue` will be returned.
    */
    template <typename T>
    const T& Get(const T& defaultValue) const;

    /// \brief Sets the value. It will be copied|moved into Any depending on lvalue|rvalue.
    /// \param [in,out] value   The value to set.
    /// \param          notAny  (Optional) Used to prevent creating Any from other Any. Shouldn't be specified by user.
    template <typename T>
    void Set(T&& value, NotAny<T> notAny = true);

    void Set(Any&& any);

    void Set(const Any& any);

    /// \brief Determine if contained value can be cast to specified T.
    /// \return true if we can be casted, false if not.
    /// \see AnyCast
    template <typename T>
    bool CanCast() const;

    /// \brief Casts contained value into value with specified type T.
    /// \exception  DAVA::Exception contained value can't be casted to value with specified type T.
    /// \return value with specified type T.
    /// \see AnyCast
    template <typename T>
    T Cast() const;

    template <typename T>
    T Cast(const T& defaultValue) const;

    /** Returns new any with same value but other type. Use this method carefully. */
    Any ReinterpretCast(const Type* type) const;

    /// \brief  Loads value into Any from specified memory location with specified Type. Loading can be done only from
    ///         types for which Type::IsTrivial is true.
    /// \param [in,out] data    Pointer on source memory, from where value should be loaded.
    /// \param          type    The type of the loading value.
    bool LoadData(void* data, const Type* type);

    /// \brief  Stores contained value into specified memory location. Storing can
    ///          be done only for values whose type Type::IsTrivial is true.
    /// \param [in,out] data    Pointer on destination memory, where contained value should be stored.
    /// \param          size    The size of the destination memory.
    bool StoreData(void* data, size_t size) const;

    /** Returns pointer on internal data. If Any is empty internal data is unspecified. */
    const void* GetData() const;

    Any& operator=(Any&&);
    Any& operator=(const Any&) = default;

    /// \brief  Equality operator. Two Any objects can be equal only if they both contain values of the same Any::type.
    ///         Values of type for which Type::IsTrivial is true are compared by ::std::memcmp function. Values with
    ///         other types can be compared only if AnyCompate class has specialization for thous type.
    /// \exception  DAVA::Exception there is no appropriate compare operation for contaited values.
    /// \sa AnyCompare
    bool operator==(const Any&) const;

    /// \brief Inequality operator.
    /// \see Any::operator==
    bool operator!=(const Any&) const;

private:
    using CompareFn = bool (*)(const Any&, const Any&);

    const Type* type = nullptr;
    AnyStorage anyStorage;
    CompareFn compareFn = nullptr;
};

/// \brief any compare.
template <typename T>
struct AnyCompare
{
    static bool IsEqual(const Any&, const Any&);
};

/// \brief any cast.
template <typename From, typename To>
struct AnyCast
{
    static void Register(To (*)(const Any&));
    static void RegisterDefault();
};

struct PointerValueAnyLess
{
    bool operator()(const Any& v1, const Any& v2) const
    {
        bool v1Empty = v1.IsEmpty();
        bool v2Empty = v2.IsEmpty();

        if (v1Empty == true && v2Empty == true)
        {
            return false;
        }

        if (v1Empty == true && v2Empty == false)
        {
            return false;
        }

        if (v1Empty == false && v2Empty == true)
        {
            return true;
        }

        DVASSERT(v1.GetType()->IsPointer() == true);
        DVASSERT(v2.GetType()->IsPointer() == true);
        return v1.Get<void*>() < v2.Get<void*>();
    }
};

} // namespace DAVA

#define __Dava_Any__
#include "Base/Private/Any_implCompare.h"
#include "Base/Private/Any_implCast.h"
#include "Base/Private/Any_impl.h"
