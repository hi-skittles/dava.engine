#include "StyleSheetRootProperty.h"

#include "SectionProperty.h"
#include "StyleSheetProperty.h"
#include "StyleSheetSelectorProperty.h"

#include "PropertyVisitor.h"
#include "Model/PackageHierarchy/StyleSheetNode.h"
#include "UI/Styles/UIStyleSheet.h"

using namespace DAVA;

StyleSheetRootProperty::StyleSheetRootProperty(StyleSheetNode* aStyleSheet, const DAVA::UIStyleSheetSourceInfo& sourceInfo, const DAVA::Vector<DAVA::UIStyleSheetSelectorChain>& selectorChains, const DAVA::Vector<DAVA::UIStyleSheetProperty>& properties)
    : styleSheet(aStyleSheet) // weak
{
    propertyTable = new UIStyleSheetPropertyTable();

    selectors = new StyleSheetSelectorsSection("Selectors");
    selectors->SetParent(this);

    for (const UIStyleSheetSelectorChain& chain : selectorChains)
    {
        ScopedPtr<StyleSheetSelectorProperty> selector(new StyleSheetSelectorProperty(chain, sourceInfo));
        selector->SetStyleSheetPropertyTable(propertyTable);
        selectors->AddProperty(selector);
    }

    propertiesSection = new StyleSheetPropertiesSection("Properties");
    propertiesSection->SetParent(this);
    for (const UIStyleSheetProperty& p : properties)
    {
        ScopedPtr<StyleSheetProperty> prop(new StyleSheetProperty(p));
        propertiesSection->AddProperty(prop);
    }

    UpdateStyleSheetPropertyTable();
}

StyleSheetRootProperty::~StyleSheetRootProperty()
{
    styleSheet = nullptr; // weak

    selectors->SetParent(nullptr);
    SafeRelease(selectors);

    propertiesSection->SetParent(nullptr);
    SafeRelease(propertiesSection);

    SafeRelease(propertyTable);
}

uint32 StyleSheetRootProperty::GetCount() const
{
    return SECTION_COUNT;
}

AbstractProperty* StyleSheetRootProperty::GetProperty(int index) const
{
    switch (index)
    {
    case SECTION_SELECTORS:
        return selectors;
    case SECTION_PROPERTIES:
        return propertiesSection;
    }
    DVASSERT(false);
    return nullptr;
}

void StyleSheetRootProperty::Accept(PropertyVisitor* visitor)
{
    visitor->VisitStyleSheetRoot(this);
}

bool StyleSheetRootProperty::IsReadOnly() const
{
    return styleSheet->IsReadOnly();
}

const DAVA::String& StyleSheetRootProperty::GetName() const
{
    static String rootName = "Style Sheets Properties";
    return rootName;
}

const Type* StyleSheetRootProperty::GetValueType() const
{
    return nullptr;
}

AbstractProperty::ePropertyType StyleSheetRootProperty::GetType() const
{
    return TYPE_HEADER;
}

void StyleSheetRootProperty::SetProperty(AbstractProperty* property, const DAVA::Any& newValue)
{
    property->SetValue(newValue);
    UpdateStyleSheetPropertyTable();

    propertyChanged.Emit(property);
}

bool StyleSheetRootProperty::CanAddProperty(DAVA::uint32 propertyIndex) const
{
    return !IsReadOnly() && FindPropertyByPropertyIndex(propertyIndex) == nullptr;
}

bool StyleSheetRootProperty::CanRemoveProperty(DAVA::uint32 propertyIndex) const
{
    return !IsReadOnly() && FindPropertyByPropertyIndex(propertyIndex) != nullptr;
}

void StyleSheetRootProperty::AddProperty(StyleSheetProperty* property)
{
    if (CanAddProperty(property->GetPropertyIndex()) && property->GetParent() == nullptr)
    {
        stylePropertyWillBeAdded.Emit(propertiesSection, property, property->GetPropertyIndex());

        uint32 index = 0;
        while (index < propertiesSection->GetCount())
        {
            StyleSheetProperty* p = propertiesSection->GetProperty(index);
            if (p->GetPropertyIndex() > property->GetPropertyIndex())
                break;
            index++;
        }
        propertiesSection->InsertProperty(property, index);
        UpdateStyleSheetPropertyTable();

        stylePropertyWasAdded.Emit(propertiesSection, property, property->GetPropertyIndex());
    }
}

void StyleSheetRootProperty::RemoveProperty(StyleSheetProperty* property)
{
    uint32 index = propertiesSection->GetIndex(property);
    if (!IsReadOnly() && index != -1)
    {
        stylePropertyWillBeRemoved.Emit(propertiesSection, property, index);

        propertiesSection->RemoveProperty(property);
        UpdateStyleSheetPropertyTable();

        stylePropertyWasRemoved.Emit(propertiesSection, property, index);
    }
}

bool StyleSheetRootProperty::CanAddSelector() const
{
    return !IsReadOnly();
}

bool StyleSheetRootProperty::CanRemoveSelector() const
{
    return !IsReadOnly() && selectors->GetCount() > 1;
}

void StyleSheetRootProperty::InsertSelector(StyleSheetSelectorProperty* property, int index)
{
    if (CanAddSelector() && selectors->GetIndex(property) == -1)
    {
        styleSelectorWillBeAdded.Emit(selectors, property, index);

        selectors->InsertProperty(property, index);
        property->SetStyleSheetPropertyTable(propertyTable);

        styleSelectorWasAdded.Emit(selectors, property, index);
    }
    else
    {
        DVASSERT(false);
    }
}

void StyleSheetRootProperty::RemoveSelector(StyleSheetSelectorProperty* property)
{
    int32 index = selectors->GetIndex(property);
    if (CanRemoveSelector() && index != -1)
    {
        styleSelectorWillBeRemoved.Emit(selectors, property, index);

        selectors->RemoveProperty(property);
        property->SetStyleSheetPropertyTable(nullptr);

        styleSelectorWasRemoved.Emit(selectors, property, index);
    }
    else
    {
        DVASSERT(false);
    }
}

StyleSheetSelectorsSection* StyleSheetRootProperty::GetSelectors() const
{
    return selectors;
}

StyleSheetPropertiesSection* StyleSheetRootProperty::GetPropertiesSection() const
{
    return propertiesSection;
}

StyleSheetProperty* StyleSheetRootProperty::FindPropertyByPropertyIndex(DAVA::uint32 propertyIndex) const
{
    for (uint32 i = 0; i < propertiesSection->GetCount(); i++)
    {
        StyleSheetProperty* p = static_cast<StyleSheetProperty*>(propertiesSection->GetProperty(i));
        if (p->GetPropertyIndex() == propertyIndex)
            return p;
    }
    return nullptr;
}

StyleSheetSelectorProperty* StyleSheetRootProperty::GetSelectorAtIndex(DAVA::int32 index) const
{
    if (0 <= index && index < static_cast<int32>(selectors->GetCount()))
    {
        return static_cast<StyleSheetSelectorProperty*>(selectors->GetProperty(index));
    }
    else
    {
        DVASSERT(false);
        return nullptr;
    }
}

String StyleSheetRootProperty::GetSelectorsAsString() const
{
    StringStream stream;
    for (uint32 i = 0; i < selectors->GetCount(); i++)
    {
        if (i > 0)
            stream << ", ";
        stream << selectors->GetProperty(i)->GetValue().Cast<String>();
    }
    return stream.str();
}

Vector<UIStyleSheet*> StyleSheetRootProperty::CollectStyleSheets()
{
    Vector<UIStyleSheet*> result;
    for (uint32 i = 0; i < selectors->GetCount(); i++)
    {
        result.push_back(selectors->GetProperty(i)->GetStyleSheet());
    }
    return result;
}

DAVA::Vector<DAVA::UIStyleSheetSelectorChain> StyleSheetRootProperty::CollectStyleSheetSelectors() const
{
    Vector<UIStyleSheetSelectorChain> result;
    for (uint32 i = 0; i < selectors->GetCount(); i++)
    {
        result.push_back(selectors->GetProperty(i)->GetSelectorChain());
    }
    return result;
}

DAVA::Vector<DAVA::UIStyleSheetProperty> StyleSheetRootProperty::CollectStyleSheetProperties() const
{
    Vector<UIStyleSheetProperty> properties;
    for (uint32 i = 0; i < propertiesSection->GetCount(); i++)
    {
        properties.push_back(propertiesSection->GetProperty(i)->GetProperty());
    }
    return properties;
}

DAVA::UIStyleSheetPropertyTable* StyleSheetRootProperty::GetStyleSheetPropertyTable() const
{
    return propertyTable;
}

void StyleSheetRootProperty::UpdateStyleSheetPropertyTable()
{
    propertyTable->SetProperties(CollectStyleSheetProperties());
}
