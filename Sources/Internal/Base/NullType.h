#ifndef __DAVAENGINE_NULLTYPE_H__
#define __DAVAENGINE_NULLTYPE_H__

namespace DAVA
{
class NullType
{
};

template <typename U>
struct IsNullType
{
    enum
    {
        result = false
    };
};

template <>
struct IsNullType<NullType>
{
    enum
    {
        result = true
    };
};
}

#endif // __DAVAENGINE_NULLTYPE_H__
