#include "DirectionBasedNavigationAlgorithm.h"

#include "UIFocusComponent.h"
#include "UIFocusGroupComponent.h"
#include "UINavigationComponent.h"
#include "FocusHelpers.h"

#include "UI/UIControl.h"
#include "UI/UIControlSystem.h"
#include "UI/UIList.h"
#include "UI/UIEvent.h"
#include "UI/UIControlHelpers.h"

namespace DAVA
{
DirectionBasedNavigationAlgorithm::DirectionBasedNavigationAlgorithm(UIControl* root_)
{
    root = root_;
}

DirectionBasedNavigationAlgorithm::~DirectionBasedNavigationAlgorithm()
{
}

UIControl* DirectionBasedNavigationAlgorithm::GetNextControl(UIControl* focusedControl, UINavigationComponent::Direction dir)
{
    if (focusedControl != nullptr)
    {
        UIControl* next = FindNextControl(focusedControl, dir);
        if (next != nullptr && next != focusedControl)
        {
            return next;
        }
    }
    return nullptr;
}

UIControl* DirectionBasedNavigationAlgorithm::FindNextControl(UIControl* focusedControl, UINavigationComponent::Direction dir) const
{
    UIControl* next = FindNextSpecifiedControl(focusedControl, dir);
    if (next != nullptr)
    {
        return next;
    }

    UIControl* parent = focusedControl;
    while (parent != nullptr && parent != root.Get())
    {
        UIFocusGroupComponent* focusGroup = parent->GetComponent<UIFocusGroupComponent>();
        if (focusGroup != nullptr)
        {
            UIControl* c = FindNearestControl(focusedControl, parent, dir);
            if (c != nullptr)
            {
                return c;
            }
        }

        parent = parent->GetParent();
    }

    if (root.Valid())
    {
        return FindNearestControl(focusedControl, root.Get(), dir);
    }

    return nullptr;
}

UIControl* DirectionBasedNavigationAlgorithm::FindNextSpecifiedControl(UIControl* focusedControl, UINavigationComponent::Direction dir) const
{
    UINavigationComponent* navigation = focusedControl->GetComponent<UINavigationComponent>();
    if (navigation != nullptr)
    {
        const String& controlInDirection = navigation->GetNextControlPathInDirection(dir);
        if (!controlInDirection.empty())
        {
            UIControl* next = UIControlHelpers::FindControlByPath(controlInDirection, focusedControl);
            if (next != nullptr && FocusHelpers::CanFocusControl(next))
            {
                return next;
            }
        }
    }
    return nullptr;
}

UIControl* DirectionBasedNavigationAlgorithm::FindNearestControl(UIControl* focusedControl, UIControl* control, UINavigationComponent::Direction dir) const
{
    UIControl* bestControl = nullptr;
    float32 bestDistSq = 0;

    Rect rect = GetRect(focusedControl);
    Vector2 pos = rect.GetCenter();

    switch (dir)
    {
    case UINavigationComponent::Direction::UP:
        pos.y = rect.y;
        break;

    case UINavigationComponent::Direction::DOWN:
        pos.y = rect.y + rect.dy;
        break;

    case UINavigationComponent::Direction::LEFT:
        pos.x = rect.x;
        break;

    case UINavigationComponent::Direction::RIGHT:
        pos.x = rect.x + rect.dx;
        break;

    default:
        DVASSERT(false);
        break;
    }

    if (CanNavigateToControl(focusedControl, control, dir))
    {
        return control;
    }

    for (const auto& c : control->GetChildren())
    {
        UIControl* res = FindNearestControl(focusedControl, c.Get(), dir);

        if (res != nullptr)
        {
            Vector2 p = CalcNearestPos(pos, res, dir);
            float32 distSq = (p - pos).SquareLength();
            if (bestControl == nullptr || distSq < bestDistSq)
            {
                bestControl = res;
                bestDistSq = distSq;
            }
        }
    }

    return bestControl;
}

Vector2 DirectionBasedNavigationAlgorithm::CalcNearestPos(const Vector2& pos, UIControl* testControl, UINavigationComponent::Direction dir) const
{
    Rect r = GetRect(testControl);
    Vector2 res = r.GetCenter();
    if (dir == UINavigationComponent::Direction::UP || dir == UINavigationComponent::Direction::DOWN)
    {
        if (pos.x > r.x + r.dx)
        {
            res.x = r.x + r.dx;
        }
        else if (pos.x < r.x)
        {
            res.x = r.x;
        }
        else
        {
            res.x = pos.x;
        }
    }
    else
    {
        if (pos.y > r.y + r.dy)
        {
            res.y = r.y + r.dy;
        }
        else if (pos.y < r.y)
        {
            res.y = r.y;
        }
        else
        {
            res.y = pos.y;
        }
    }

    if (dir == UINavigationComponent::Direction::UP)
    {
        res.y = Min(r.y + r.dy, pos.y);
    }
    else if (dir == UINavigationComponent::Direction::DOWN)
    {
        res.y = Max(r.y, pos.y);
    }
    else if (dir == UINavigationComponent::Direction::LEFT)
    {
        res.x = Min(r.x + r.dx, pos.x);
    }
    else if (dir == UINavigationComponent::Direction::RIGHT)
    {
        res.x = Max(r.x, pos.x);
    }

    return res;
}

bool DirectionBasedNavigationAlgorithm::CanNavigateToControl(UIControl* focusedControl, UIControl* control, UINavigationComponent::Direction dir) const
{
    if (control == focusedControl || !FocusHelpers::CanFocusControl(control))
    {
        return false;
    }
    Rect rect = GetRect(focusedControl);
    Vector2 pos = rect.GetCenter();
    Vector2 srcPos = rect.GetCenter();

    Vector2 cPos = CalcNearestPos(pos, control, dir);

    float32 dx = cPos.x - srcPos.x;
    float32 dy = cPos.y - srcPos.y;

    switch (dir)
    {
    case UINavigationComponent::Direction::UP:
        return dy < 0 && Abs(dy) > Abs(dx);

    case UINavigationComponent::Direction::DOWN:
        return dy > 0 && Abs(dy) > Abs(dx);

    case UINavigationComponent::Direction::LEFT:
        return dx < 0 && Abs(dx) > Abs(dy);

    case UINavigationComponent::Direction::RIGHT:
        return dx > 0 && Abs(dx) > Abs(dy);

    default:
        DVASSERT(false);
        break;
    }
    return false;
}

Rect DirectionBasedNavigationAlgorithm::GetRect(UIControl* control) const
{
    return control->GetGeometricData().GetUnrotatedRect();
}
}
