#ifndef __DAVAENGINE_INTROSPECTION_FLAGS_H__
#define __DAVAENGINE_INTROSPECTION_FLAGS_H__

#include "Base/BaseTypes.h"

namespace DAVA
{
enum eIntrospectionFlags : uint32
{
    I_NONE = 0x00,

    I_VIEW = 0x01, // this member can be view by user
    I_EDIT = 0x02, // this member can be edited by user

    I_SAVE = 0x04, // this member should be saved during serialization
    I_LOAD = 0x08, // this member should be loaded during serialization

    I_PREFERENCE = 0x10, // this member should be used as preference
};
};

#endif // __DAVAENGINE_INTROSPECTION_FLAGS_H__
