#include "TabTraversalAlgorithm.h"

#include "UI/UIControl.h"
#include "UI/Focus/FocusHelpers.h"

namespace DAVA
{
TabTraversalAlgorithm::TabTraversalAlgorithm(UIControl* root_)
{
    root = root_;
}

TabTraversalAlgorithm::~TabTraversalAlgorithm()
{
}

UIControl* TabTraversalAlgorithm::GetNextControl(UIControl* focusedControl, UITabOrderComponent::Direction dir, bool repeat)
{
    if (focusedControl != nullptr && root != focusedControl)
    {
        UIControl* parent = focusedControl->GetParent();

        if (parent != nullptr)
        {
            Vector<RefPtr<UIControl>> children;
            PrepareChildren(parent, children);

            UIControl* res = nullptr;
            if (dir == UITabOrderComponent::FORWARD)
            {
                res = FindNextControl(focusedControl, children.begin(), children.end(), dir);
            }
            else
            {
                res = FindNextControl(focusedControl, children.rbegin(), children.rend(), dir);
            }

            if (res != nullptr)
            {
                return res;
            }

            res = GetNextControl(parent, dir, repeat);
            if (res != nullptr)
            {
                return res;
            }

            if (repeat)
            {
                return FindFirstControl(parent, dir);
            }
        }
    }
    return nullptr;
}

template <typename It>
UIControl* TabTraversalAlgorithm::FindNextControl(UIControl* focusedControl, It begin, It end, UITabOrderComponent::Direction dir)
{
    auto it = begin;
    while (it != end && *it != focusedControl)
    {
        ++it;
    }

    if (it == end)
    {
        return nullptr;
    }

    ++it;

    for (; it != end; ++it)
    {
        UIControl* res = FindFirstControl(it->Get(), dir);
        if (res != nullptr)
        {
            return res;
        }
    }

    return nullptr;
}

UIControl* TabTraversalAlgorithm::FindFirstControl(UIControl* control, UITabOrderComponent::Direction dir)
{
    if (FocusHelpers::CanFocusControl(control))
    {
        return control;
    }

    Vector<RefPtr<UIControl>> children;
    PrepareChildren(control, children);

    if (dir == UITabOrderComponent::FORWARD)
    {
        return FindFirstControlRecursive(children.begin(), children.end(), dir);
    }
    else
    {
        return FindFirstControlRecursive(children.rbegin(), children.rend(), dir);
    }
}

template <typename It>
UIControl* TabTraversalAlgorithm::FindFirstControlRecursive(It begin, It end, UITabOrderComponent::Direction dir)
{
    for (auto it = begin; it != end; ++it)
    {
        UIControl* res = FindFirstControl(it->Get(), dir);
        if (res)
        {
            return res;
        }
    }
    return nullptr;
}

void TabTraversalAlgorithm::PrepareChildren(UIControl* control, Vector<RefPtr<UIControl>>& children)
{
    DVASSERT(children.empty());

    children.reserve(control->GetChildren().size());
    children.insert(children.end(), control->GetChildren().begin(), control->GetChildren().end());

    std::stable_sort(children.begin(), children.end(), [](const RefPtr<UIControl>& c1, const RefPtr<UIControl>& c2) {
        UITabOrderComponent* f1 = c1->GetComponent<UITabOrderComponent>();
        if (f1 == nullptr)
        {
            return false;
        }

        UITabOrderComponent* f2 = c2->GetComponent<UITabOrderComponent>();
        return f2 == nullptr || f1->GetTabOrder() < f2->GetTabOrder(); // important: f1 != nullptr
    });
}
}
