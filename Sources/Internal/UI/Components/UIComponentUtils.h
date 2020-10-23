#pragma once

#include "Base/BaseTypes.h"
#include "Base/Type.h"

namespace DAVA
{
class UIComponentUtils
{
public:
    static bool IsMultiple(const Type* componentType);
    static bool IsHidden(const Type* componentType);
    static String GetDisplayName(const Type* componentType);
    static String GetGroupName(const Type* componentType);
};
}
