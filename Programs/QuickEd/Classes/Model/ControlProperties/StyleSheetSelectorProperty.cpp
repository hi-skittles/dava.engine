#include "StyleSheetSelectorProperty.h"

#include "PropertyVisitor.h"
#include "../PackageHierarchy/StyleSheetNode.h"

#include "UI/Styles/UIStyleSheet.h"
#include "Utils/Utils.h"

using namespace DAVA;

StyleSheetSelectorProperty::StyleSheetSelectorProperty(const UIStyleSheetSelectorChain& chain, const UIStyleSheetSourceInfo& sourceInfo_)
    : ValueProperty("Selector", Type::Instance<String>())
{
    styleSheet = new UIStyleSheet();
    styleSheet->SetSelectorChain(chain);
    styleSheet->SetSourceInfo(sourceInfo_);

    SetOverridden(true);
    value = chain.ToString();
}

StyleSheetSelectorProperty::~StyleSheetSelectorProperty()
{
    SafeRelease(styleSheet);
}

uint32 StyleSheetSelectorProperty::GetCount() const
{
    return 0;
}

AbstractProperty* StyleSheetSelectorProperty::GetProperty(int index) const
{
    return nullptr;
}

void StyleSheetSelectorProperty::Accept(PropertyVisitor* visitor)
{
    visitor->VisitStyleSheetSelectorProperty(this);
}

AbstractProperty::ePropertyType StyleSheetSelectorProperty::GetType() const
{
    return TYPE_VARIANT;
}

uint32 StyleSheetSelectorProperty::GetFlags() const
{
    return EF_CAN_REMOVE;
}

Any StyleSheetSelectorProperty::GetValue() const
{
    return Any(value);
}

void StyleSheetSelectorProperty::ApplyValue(const DAVA::Any& aValue)
{
    Vector<String> selectorList;
    Split(aValue.Cast<String>(), ",", selectorList);

    if (!selectorList.empty())
    {
        styleSheet->SetSelectorChain(UIStyleSheetSelectorChain(selectorList.front()));
    }
    else
    {
        styleSheet->SetSelectorChain(UIStyleSheetSelectorChain(""));
    }
    value = styleSheet->GetSelectorChain().ToString();
}

const DAVA::UIStyleSheetSelectorChain& StyleSheetSelectorProperty::GetSelectorChain() const
{
    return styleSheet->GetSelectorChain();
}

const DAVA::String& StyleSheetSelectorProperty::GetSelectorChainString() const
{
    return value;
}

UIStyleSheet* StyleSheetSelectorProperty::GetStyleSheet() const
{
    return styleSheet;
}

void StyleSheetSelectorProperty::SetStyleSheetPropertyTable(DAVA::UIStyleSheetPropertyTable* propertyTable)
{
    styleSheet->SetPropertyTable(propertyTable);
}
