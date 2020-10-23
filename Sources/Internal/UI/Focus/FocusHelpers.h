#ifndef __DAVAENGINE_FOCUS_HELPERS_H__
#define __DAVAENGINE_FOCUS_HELPERS_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Math/Vector.h"

namespace DAVA
{
class UIControl;

class FocusHelpers
{
public:
    static bool CanFocusControl(UIControl* control);
};
}


#endif //__DAVAENGINE_FOCUS_HELPERS_H__
