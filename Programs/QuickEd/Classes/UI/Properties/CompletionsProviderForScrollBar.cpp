#include "CompletionsProviderForScrollBar.h"

#include "Model/ControlProperties/AbstractProperty.h"
#include "Model/ControlProperties/RootProperty.h"
#include "Model/PackageHierarchy/ControlNode.h"
#include "Model/PackageHierarchy/PackageNode.h"
#include "Model/PackageHierarchy/PackageNode.h"

#include "UI/UIControl.h"
#include "UI/UIControlHelpers.h"
#include "UI/UIScrollBarDelegate.h"

using namespace DAVA;

CompletionsProviderForScrollBar::CompletionsProviderForScrollBar()
{
}

CompletionsProviderForScrollBar::~CompletionsProviderForScrollBar()
{
}

QStringList CompletionsProviderForScrollBar::GetCompletions(AbstractProperty* property)
{
    QStringList list;

    RootProperty* root = dynamic_cast<RootProperty*>(property->GetRootProperty());
    if (root)
    {
        PackageBaseNode* currentNode = root->GetControlNode();
        PackageBaseNode* rootNode = currentNode;

        if (rootNode != nullptr)
        {
            PackageNode* package = rootNode->GetPackage();

            if (package != nullptr)
            {
                while (rootNode->GetParent() != nullptr && rootNode->GetParent()->GetControl() != nullptr)
                {
                    rootNode = rootNode->GetParent();
                }
            }

            CollectCompletions(list, currentNode, rootNode);
        }
    }

    return list;
}

void CompletionsProviderForScrollBar::CollectCompletions(QStringList& list, PackageBaseNode* src, PackageBaseNode* node)
{
    UIControl* control = node->GetControl();
    if (control != nullptr)
    {
        if (dynamic_cast<UIScrollBarDelegate*>(control) != nullptr)
        {
            list << QString::fromStdString(UIControlHelpers::GetPathToOtherControl(src->GetControl(), node->GetControl()));
        }
    }
    else
    {
        DVASSERT(false);
    }

    for (int32 i = 0; i < node->GetCount(); i++)
    {
        CollectCompletions(list, src, node->Get(i));
    }
}
