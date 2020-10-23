#include "UI/DataBinding/Private/UIDataNode.h"

#include "UI/DataBinding/UIDataBindingIssueDelegate.h"
#include "UI/Components/UIComponent.h"

namespace DAVA
{
UIDataNode::UIDataNode(bool editorMode_)
    : editorMode(editorMode_)
{
}

UIDataNode::~UIDataNode()
{
    ResetError();
}

UIDataModel* UIDataNode::GetParent() const
{
    return parent;
}

void UIDataNode::SetParent(UIDataModel* parent_)
{
    parent = parent_;
}

void UIDataNode::SetIssueDelegate(UIDataBindingIssueDelegate* issueDelegate_)
{
    issueDelegate = issueDelegate_;
}

int32 UIDataNode::GetDepencencyId() const
{
    return dependencyId;
}

void UIDataNode::NotifyError(const String& message, const String& property)
{
    if (issueDelegate != nullptr)
    {
        if (issueId == NO_ISSUE)
        {
            issueId = issueDelegate->GenerateNewId();
            issueDelegate->OnIssueAdded(issueId, message, GetComponent()->GetControl(), property);
        }
        else
        {
            issueDelegate->OnIssueChanged(issueId, message);
        }
    }
}

void UIDataNode::ResetError()
{
    if (issueId != NO_ISSUE)
    {
        if (issueDelegate != nullptr)
        {
            issueDelegate->OnIssueRemoved(issueId);
        }
        issueId = NO_ISSUE;
    }
}

UIDataErrorGuard::UIDataErrorGuard(UIDataNode* dataNode_)
    : dataNode(dataNode_)
{
}

UIDataErrorGuard::~UIDataErrorGuard()
{
    if (hasToResetError)
    {
        dataNode->ResetError();
    }
}

void UIDataErrorGuard::NotifyError(const String& message, const String& property)
{
    hasToResetError = false;
    dataNode->NotifyError(message, property);
}
}
