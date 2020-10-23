#ifndef __DAVAENGINE_TAB_TRAVERSAL_ALGORITHM_H__
#define __DAVAENGINE_TAB_TRAVERSAL_ALGORITHM_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Math/Vector.h"
#include "UI/Focus/UITabOrderComponent.h"

namespace DAVA
{
class UIControl;
class UIList;
class UIEvent;

class TabTraversalAlgorithm
{
public:
    TabTraversalAlgorithm(UIControl* root);
    ~TabTraversalAlgorithm();

    UIControl* GetNextControl(UIControl* focusedControl, UITabOrderComponent::Direction dir, bool repeat);

private:
    template <typename It>
    UIControl* FindNextControl(UIControl* focusedControl, It begin, It end, UITabOrderComponent::Direction dir);

    UIControl* FindFirstControl(UIControl* control, UITabOrderComponent::Direction dir);
    template <typename It>
    UIControl* FindFirstControlRecursive(It begin, It end, UITabOrderComponent::Direction dir);

    void PrepareChildren(UIControl* control, Vector<RefPtr<UIControl>>& children);

    RefPtr<UIControl> root;
};
}

#endif // __DAVAENGINE_TAB_TRAVERSAL_ALGORITHM_H__
