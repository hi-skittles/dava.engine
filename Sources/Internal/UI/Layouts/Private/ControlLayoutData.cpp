#include "ControlLayoutData.h"

#include "UI/UIControl.h"
#include "UI/Components/UIComponent.h"
#include "UI/Layouts/UIIgnoreLayoutComponent.h"
#include "UI/Layouts/UILayoutSourceRectComponent.h"

namespace DAVA
{
ControlLayoutData::ControlLayoutData(UIControl* control_)
    : control(control_)
{
    UILayoutSourceRectComponent* sourceRectComponent = control->GetComponent<UILayoutSourceRectComponent>();

    if (sourceRectComponent != nullptr)
    {
        position = sourceRectComponent->GetPosition() - control->GetPivotPoint();
        size = sourceRectComponent->GetSize();
    }
    else
    {
        position = control->GetPosition() - control->GetPivotPoint();
        size = control->GetSize();
    }
}

void ControlLayoutData::ApplyLayoutToControl()
{
    UILayoutSourceRectComponent* sourceRectComponent = control->GetComponent<UILayoutSourceRectComponent>();
    if (HasFlag(FLAG_SIZE_CHANGED))
    {
        control->SetSize(size);
        control->OnSizeChanged();
    }
    else if (sourceRectComponent != nullptr)
    {
        control->SetSize(sourceRectComponent->GetSize());
        control->OnSizeChanged();
    }

    if (HasFlag(FLAG_POSITION_CHANGED))
    {
        control->SetPosition(position + control->GetPivotPoint());
    }
    else if (sourceRectComponent != nullptr)
    {
        control->SetPosition(sourceRectComponent->GetPosition());
    }

    control->ResetLayoutDirty();
}

void ControlLayoutData::ApplyOnlyPositionLayoutToControl()
{
    UILayoutSourceRectComponent* sourceRectComponent = control->GetComponent<UILayoutSourceRectComponent>();
    if (HasFlag(FLAG_POSITION_CHANGED))
    {
        control->SetPosition(position + control->GetPivotPoint());
    }
    else if (sourceRectComponent != nullptr)
    {
        control->SetPosition(sourceRectComponent->GetPosition());
    }

    control->ResetLayoutPositionDirty();
}

UIControl* ControlLayoutData::GetControl() const
{
    return control;
}

bool ControlLayoutData::HasFlag(eFlag flag) const
{
    return (flags & flag) == flag;
}

void ControlLayoutData::SetFlag(eFlag flag)
{
    flags |= flag;
}

int32 ControlLayoutData::GetParentIndex() const
{
    return parent;
}

void ControlLayoutData::SetParentIndex(int32 index)
{
    parent = index;
}

int32 ControlLayoutData::GetFirstChildIndex() const
{
    return firstChild;
}

void ControlLayoutData::SetFirstChildIndex(int32 index)
{
    firstChild = index;
}

int32 ControlLayoutData::GetLastChildIndex() const
{
    return lastChild;
}

void ControlLayoutData::SetLastChildIndex(int32 index)
{
    lastChild = index;
}

bool ControlLayoutData::HasChildren() const
{
    return lastChild >= firstChild;
}

float32 ControlLayoutData::GetSize(Vector2::eAxis axis) const
{
    return size.data[axis];
}

void ControlLayoutData::SetSize(Vector2::eAxis axis, float32 value)
{
    size.data[axis] = value;
    flags |= FLAG_SIZE_CHANGED;
}

void ControlLayoutData::SetSizeWithoutChangeFlag(Vector2::eAxis axis, float32 value)
{
    size.data[axis] = value;
}

float32 ControlLayoutData::GetPosition(Vector2::eAxis axis) const
{
    return position.data[axis];
}

void ControlLayoutData::SetPosition(Vector2::eAxis axis, float32 value)
{
    position.data[axis] = value;
    flags |= FLAG_POSITION_CHANGED;
}

float32 ControlLayoutData::GetX() const
{
    return position.x;
}

float32 ControlLayoutData::GetY() const
{
    return position.y;
}

float32 ControlLayoutData::GetWidth() const
{
    return size.dx;
}

float32 ControlLayoutData::GetHeight() const
{
    return size.dy;
}

bool ControlLayoutData::HaveToSkipControl(bool skipInvisible) const
{
    if (skipInvisible && !control->GetVisibilityFlag())
        return true;

    UIIgnoreLayoutComponent* ignoreComponent = control->GetComponent<UIIgnoreLayoutComponent>();
    return ignoreComponent != nullptr && ignoreComponent->IsEnabled();
}
}
