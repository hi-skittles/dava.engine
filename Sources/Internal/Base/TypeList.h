#ifndef __DAVAENGINE_TYPELIST_H__
#define __DAVAENGINE_TYPELIST_H__

#include "NullType.h"

namespace DAVA
{
// type list
template <class T, class U>
struct Typelist
{
    using Head = T;
    using Tail = U;
};

// type list operations
namespace TL
{
// append type
template <class TList, class T>
struct Append;

template <>
struct Append<NullType, NullType>
{
    using Result = NullType;
};

template <class T>
struct Append<NullType, T>
{
    using Result = Typelist<T, NullType>;
};

template <class Head, class Tail>
struct Append<NullType, Typelist<Head, Tail>>
{
    using Result = Typelist<Head, Tail>;
};

template <class Head, class Tail, class T>
struct Append<Typelist<Head, Tail>, T>
{
    using Result = Typelist<Head, typename Append<Tail, T>::Result>;
};

template <class Head, class Tail>
struct Append<Typelist<Head, Tail>, NullType>
{
    using Result = Typelist<Head, Tail>;
};

// type at given index
template <class TList, unsigned int index>
struct TypeAt;

template <class Head, class Tail>
struct TypeAt<Typelist<Head, Tail>, 0>
{
    using Result = Head;
};

template <class Head, class Tail, unsigned int i>
struct TypeAt<Typelist<Head, Tail>, i>
{
    using Result = typename TypeAt<Tail, i - 1>::Result;
};

// type at given index with default type, when no such index
template <class TList, unsigned int index, typename DefaultType = NullType>
struct TypeAtNonStrict
{
    using Result = DefaultType;
};

template <class Head, class Tail, typename DefaultType>
struct TypeAtNonStrict<Typelist<Head, Tail>, 0, DefaultType>
{
    using Result = Head;
};

template <class Head, class Tail, unsigned int i, typename DefaultType>
struct TypeAtNonStrict<Typelist<Head, Tail>, i, DefaultType>
{
    using Result = typename TypeAtNonStrict<Tail, i - 1, DefaultType>::Result;
};

// finds the index of a given type
template <class TList, class T>
struct IndexOf;

template <class T>
struct IndexOf<NullType, T>
{
    enum
    {
        value = -1
    };
};

template <class T, class Tail>
struct IndexOf<Typelist<T, Tail>, T>
{
    enum
    {
        value = 0
    };
};

template <class Head, class Tail, class T>
struct IndexOf<Typelist<Head, Tail>, T>
{
private:
    enum
    {
        temp = IndexOf<Tail, T>::value
    };

public:
    enum
    {
        value = (temp == -1 ? -1 : 1 + temp)
    };
};
};
}

#define DAVA_TYPELIST_1(T1) DAVA::Typelist<T1, NullType>
#define DAVA_TYPELIST_2(T1, T2) DAVA::Typelist<T1, DAVA_TYPELIST_1(T2)>
#define DAVA_TYPELIST_3(T1, T2, T3) DAVA::Typelist<T1, DAVA_TYPELIST_2(T2, T3)>
#define DAVA_TYPELIST_4(T1, T2, T3, T4) DAVA::Typelist<T1, DAVA_TYPELIST_3(T2, T3, T4)>
#define DAVA_TYPELIST_5(T1, T2, T3, T4, T5) DAVA::Typelist<T1, DAVA_TYPELIST_4(T2, T3, T4, T5)>
#define DAVA_TYPELIST_6(T1, T2, T3, T4, T5, T6) DAVA::Typelist<T1, DAVA_TYPELIST_5(T2, T3, T4, T5, T6)>
#define DAVA_TYPELIST_7(T1, T2, T3, T4, T5, T6, T7) DAVA::Typelist<T1, DAVA_TYPELIST_6(T2, T3, T4, T5, T6, T7)>
#define DAVA_TYPELIST_8(T1, T2, T3, T4, T5, T6, T7, T8) DAVA::Typelist<T1, DAVA_TYPELIST_7(T2, T3, T4, T5, T6, T7, T8)>
#define DAVA_TYPELIST_9(T1, T2, T3, T4, T5, T6, T7, T8, T9) DAVA::Typelist<T1, DAVA_TYPELIST_8(T2, T3, T4, T5, T6, T7, T8, T9)>
#define DAVA_TYPELIST_10(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10) DAVA::Typelist<T1, DAVA_TYPELIST_9(T2, T3, T4, T5, T6, T7, T8, T9, T10)>
#define DAVA_TYPELIST_11(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11) DAVA::Typelist<T1, DAVA_TYPELIST_10(T2, T3, T4, T5, T6, T7, T8, T9, T10, T11)>
#define DAVA_TYPELIST_12(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12) DAVA::Typelist<T1, DAVA_TYPELIST_11(T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12)>
#define DAVA_TYPELIST_13(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13) DAVA::Typelist<T1, DAVA_TYPELIST_12(T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13)>
#define DAVA_TYPELIST_14(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14) DAVA::Typelist<T1, DAVA_TYPELIST_13(T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14)>
#define DAVA_TYPELIST_15(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15) DAVA::Typelist<T1, DAVA_TYPELIST_14(T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15)>
#define DAVA_TYPELIST_16(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16) DAVA::Typelist<T1, DAVA_TYPELIST_15(T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16)>

#endif // __DAVAENGINE_TYPELIST_H__
