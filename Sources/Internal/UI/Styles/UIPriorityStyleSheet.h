#ifndef __DAVAENGINE_UI_PRIORITY_STYLESHEET_H__
#define __DAVAENGINE_UI_PRIORITY_STYLESHEET_H__

#include "Base/BaseTypes.h"
#include "Base/RefPtr.h"
#include "UI/Styles/UIStyleSheet.h"

namespace DAVA
{
class UIPriorityStyleSheet
{
public:
    UIPriorityStyleSheet() = default;
    UIPriorityStyleSheet(UIStyleSheet* styleSheet, int32 priority = 0);
    UIPriorityStyleSheet(const UIPriorityStyleSheet& other);
    virtual ~UIPriorityStyleSheet();

    UIPriorityStyleSheet& operator=(const UIPriorityStyleSheet& other);
    bool operator<(const UIPriorityStyleSheet& other) const;

    inline UIStyleSheet* GetStyleSheet() const;
    inline int32 GetPriority() const;

private:
    RefPtr<UIStyleSheet> styleSheet;
    int32 priority = 0;
};

UIStyleSheet* UIPriorityStyleSheet::GetStyleSheet() const
{
    return styleSheet.Get();
}

int32 UIPriorityStyleSheet::GetPriority() const
{
    return priority;
}
};


#endif // __DAVAENGINE_UI_PRIORITY_STYLESHEET_H__
