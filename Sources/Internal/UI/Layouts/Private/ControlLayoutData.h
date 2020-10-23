#ifndef __DAVAENGINE_CONTROL_LAYOUT_DATA_H__
#define __DAVAENGINE_CONTROL_LAYOUT_DATA_H__

#include "Base/BaseTypes.h"
#include "Math/Vector.h"

namespace DAVA
{
class UIControl;

class ControlLayoutData
{
public:
    enum eFlag
    {
        FLAG_NONE = 0,
        FLAG_SIZE_CHANGED = 1 << 0,
        FLAG_POSITION_CHANGED = 1 << 1,
        FLAG_SIZE_CALCULATED = 1 << 2,
        FLAG_LAST_IN_LINE = 1 << 3,
        FLAG_STICK_THIS = 1 << 4,
        FLAG_STICK_HARD = 1 << 5,
        FLAG_LTR = 1 << 6,
        FLAG_RTL = 1 << 7
    };

public:
    ControlLayoutData(UIControl* control_);

    void ApplyLayoutToControl();
    void ApplyOnlyPositionLayoutToControl();

    UIControl* GetControl() const;

    bool HasFlag(eFlag flag) const;
    void SetFlag(eFlag flag);

    int32 GetParentIndex() const;
    void SetParentIndex(int32 index);

    int32 GetFirstChildIndex() const;
    void SetFirstChildIndex(int32 index);

    int32 GetLastChildIndex() const;
    void SetLastChildIndex(int32 index);

    bool HasChildren() const;

    float32 GetSize(Vector2::eAxis axis) const;
    void SetSize(Vector2::eAxis axis, float32 value);
    void SetSizeWithoutChangeFlag(Vector2::eAxis axis, float32 value);

    float32 GetPosition(Vector2::eAxis axis) const;
    void SetPosition(Vector2::eAxis axis, float32 value);

    float32 GetX() const;
    float32 GetY() const;
    float32 GetWidth() const;
    float32 GetHeight() const;

    bool HaveToSkipControl(bool skipInvisible) const;

private:
    UIControl* control;
    int32 flags = FLAG_NONE;
    int32 parent = -1;
    int32 firstChild = 0;
    int32 lastChild = -1;
    Vector2 size;
    Vector2 position;
};
}


#endif //__DAVAENGINE_CONTROL_LAYOUT_DATA_H__
