#pragma once

#include "Base/BaseTypes.h"
#include "UI/DataBinding/Private/UIDataBindingDependenciesManager.h"

namespace DAVA
{
class UIDataModel;
class UIDataBindingIssueDelegate;
class UIComponent;

class UIDataErrorGuard;

class UIDataNode
{
    friend class UIDataErrorGuard;

public:
    UIDataNode(bool editorMode);
    virtual ~UIDataNode();

    virtual UIComponent* GetComponent() const = 0;

    UIDataModel* GetParent() const;
    void SetParent(UIDataModel* parent);

    void SetIssueDelegate(UIDataBindingIssueDelegate* issueDelegate);

    int32 GetDepencencyId() const;

protected:
    static const int32 NO_ISSUE = -1;

    void NotifyError(const String& message, const String& property);
    void ResetError();

    UIDataModel* parent = nullptr;
    UIDataBindingIssueDelegate* issueDelegate = nullptr;
    int32 issueId = NO_ISSUE;
    int32 dependencyId = UIDataBindingDependenciesManager::UNKNOWN_DEPENDENCY;
    bool editorMode = false;
};

class UIDataErrorGuard
{
public:
    UIDataErrorGuard(UIDataNode* dataNode_);
    ~UIDataErrorGuard();

    void NotifyError(const String& message, const String& property);

private:
    UIDataNode* dataNode = nullptr;
    bool hasToResetError = true;
};
}
