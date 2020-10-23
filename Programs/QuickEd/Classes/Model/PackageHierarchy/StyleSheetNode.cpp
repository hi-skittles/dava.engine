#include "StyleSheetNode.h"
#include "PackageVisitor.h"

#include "Model/ControlProperties/StyleSheetRootProperty.h"
#include "Model/ControlProperties/StyleSheetSelectorProperty.h"
#include "Model/ControlProperties/StyleSheetProperty.h"
#include "Model/ControlProperties/SectionProperty.h"

#include "UI/Styles/UIStyleSheet.h"

using namespace DAVA;

StyleSheetNode::StyleSheetNode(const UIStyleSheetSourceInfo& aSourceInfo, const DAVA::Vector<DAVA::UIStyleSheetSelectorChain>& selectorChains, const DAVA::Vector<DAVA::UIStyleSheetProperty>& properties)
    : PackageBaseNode(nullptr)
    , sourceInfo(aSourceInfo)
    , rootProperty(nullptr)
{
    rootProperty = new StyleSheetRootProperty(this, sourceInfo, selectorChains, properties);
    name = rootProperty->GetSelectorsAsString();
}

StyleSheetNode::~StyleSheetNode()
{
    SafeRelease(rootProperty);
}

StyleSheetNode* StyleSheetNode::Clone() const
{
    Vector<UIStyleSheetSelectorChain> selectors = rootProperty->CollectStyleSheetSelectors();
    Vector<UIStyleSheetProperty> properties = rootProperty->CollectStyleSheetProperties();
    return new StyleSheetNode(sourceInfo, selectors, properties);
}

int StyleSheetNode::GetCount() const
{
    return 0;
}

PackageBaseNode* StyleSheetNode::Get(int index) const
{
    return nullptr;
}

void StyleSheetNode::Accept(PackageVisitor* visitor)
{
    visitor->VisitStyleSheet(this);
}

String StyleSheetNode::GetName() const
{
    return name;
}

void StyleSheetNode::UpdateName()
{
    name = rootProperty->GetSelectorsAsString();
}

bool StyleSheetNode::CanRemove() const
{
    return !IsReadOnly();
}

bool StyleSheetNode::CanCopy() const
{
    return true;
}

StyleSheetRootProperty* StyleSheetNode::GetRootProperty() const
{
    return rootProperty;
}
