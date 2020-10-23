#ifndef __DAVAENGINE_DIRECTION_BASED_NAVIGATION_ALGORITHM_H__
#define __DAVAENGINE_DIRECTION_BASED_NAVIGATION_ALGORITHM_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Math/Vector.h"
#include "Math/Rect.h"
#include "UI/Focus/UINavigationComponent.h"

namespace DAVA
{
class UIControl;
class UIList;
class UIEvent;

class DirectionBasedNavigationAlgorithm
{
public:
    DirectionBasedNavigationAlgorithm(UIControl* root);
    ~DirectionBasedNavigationAlgorithm();

    UIControl* GetNextControl(UIControl* focusedControl, UINavigationComponent::Direction dir);

private:
    UIControl* FindFirstControl(UIControl* control) const;
    UIControl* FindNextControl(UIControl* focusedControl, UINavigationComponent::Direction dir) const;
    UIControl* FindNextSpecifiedControl(UIControl* focusedControl, UINavigationComponent::Direction dir) const;
    UIControl* FindNearestControl(UIControl* focusedControl, UIControl* control, UINavigationComponent::Direction dir) const;
    Vector2 CalcNearestPos(const Vector2& pos, UIControl* testControl, UINavigationComponent::Direction dir) const;

    bool CanNavigateToControl(UIControl* focusedControl, UIControl* control, UINavigationComponent::Direction dir) const;
    UIControl* FindFirstControlImpl(UIControl* control, UIControl* candidate) const;

    Rect GetRect(UIControl* control) const;

    RefPtr<UIControl> root;
};
}


#endif //__DAVAENGINE_DIRECTION_BASED_NAVIGATION_ALGORITHM_H__
