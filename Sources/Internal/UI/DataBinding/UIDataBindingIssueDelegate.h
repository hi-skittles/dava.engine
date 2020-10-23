#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
class UIControl;
class UIComponent;

class UIDataBindingIssueDelegate
{
public:
    virtual ~UIDataBindingIssueDelegate() = default;

    virtual int32 GenerateNewId() = 0;
    virtual void OnIssueAdded(int32 id, const String& message, const UIControl* control, const String& propertyName) = 0;
    virtual void OnIssueChanged(int32 id, const String& message) = 0;
    virtual void OnIssueRemoved(int32 id) = 0;
};
}
