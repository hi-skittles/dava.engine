#ifndef __QUICKED_STYLE_SHEETS_ROOT_PROPERTY_H__
#define __QUICKED_STYLE_SHEETS_ROOT_PROPERTY_H__

#include "AbstractProperty.h"
#include "SectionProperty.h"
#include "StyleSheetProperty.h"
#include "StyleSheetSelectorProperty.h"

#include "UI/Styles/UIStyleSheetSelectorChain.h"

#include <Functional/Signal.h>

class ValueProperty;

class StyleSheetNode;

namespace DAVA
{
class UIControl;
class UIStyleSheetPropertyTable;
class UIStyleSheet;
}

class StyleSheetPropertiesSection : public SectionProperty<StyleSheetProperty>
{
public:
    StyleSheetPropertiesSection(const DAVA::String& name)
        : SectionProperty<StyleSheetProperty>(name)
    {
    }
};

class StyleSheetSelectorsSection : public SectionProperty<StyleSheetSelectorProperty>
{
public:
    StyleSheetSelectorsSection(const DAVA::String& name)
        : SectionProperty<StyleSheetSelectorProperty>(name)
    {
    }
};

class StyleSheetRootProperty : public AbstractProperty
{
public:
    StyleSheetRootProperty(StyleSheetNode* styleSheet, const DAVA::UIStyleSheetSourceInfo& sourceInfo, const DAVA::Vector<DAVA::UIStyleSheetSelectorChain>& selectorChains, const DAVA::Vector<DAVA::UIStyleSheetProperty>& properties);

protected:
    virtual ~StyleSheetRootProperty();

public:
    DAVA::uint32 GetCount() const override;
    AbstractProperty* GetProperty(int index) const override;

    void Accept(PropertyVisitor* visitor) override;
    bool IsReadOnly() const override;

    const DAVA::String& GetName() const override;
    const DAVA::Type* GetValueType() const override;
    ePropertyType GetType() const override;

    void SetProperty(AbstractProperty* property, const DAVA::Any& newValue);
    bool CanAddProperty(DAVA::uint32 propertyIndex) const;
    bool CanRemoveProperty(DAVA::uint32 propertyIndex) const;
    void AddProperty(StyleSheetProperty* property);
    void RemoveProperty(StyleSheetProperty* property);
    bool CanAddSelector() const;
    bool CanRemoveSelector() const;
    void InsertSelector(StyleSheetSelectorProperty* property, int index);
    void RemoveSelector(StyleSheetSelectorProperty* property);

    StyleSheetSelectorsSection* GetSelectors() const;
    StyleSheetPropertiesSection* GetPropertiesSection() const;

    StyleSheetProperty* FindPropertyByPropertyIndex(DAVA::uint32 index) const;
    StyleSheetSelectorProperty* GetSelectorAtIndex(DAVA::int32 index) const;

    DAVA::String GetSelectorsAsString() const;

    DAVA::Vector<DAVA::UIStyleSheet*> CollectStyleSheets();

    DAVA::Vector<DAVA::UIStyleSheetSelectorChain> CollectStyleSheetSelectors() const;
    DAVA::Vector<DAVA::UIStyleSheetProperty> CollectStyleSheetProperties() const;

    DAVA::UIStyleSheetPropertyTable* GetStyleSheetPropertyTable() const;

    DAVA::Signal<AbstractProperty*> propertyChanged;
    DAVA::Signal<StyleSheetPropertiesSection*, StyleSheetProperty*, DAVA::int32> stylePropertyWillBeAdded;
    DAVA::Signal<StyleSheetPropertiesSection*, StyleSheetProperty*, DAVA::int32> stylePropertyWasAdded;
    DAVA::Signal<StyleSheetPropertiesSection*, StyleSheetProperty*, DAVA::int32> stylePropertyWillBeRemoved;
    DAVA::Signal<StyleSheetPropertiesSection*, StyleSheetProperty*, DAVA::int32> stylePropertyWasRemoved;
    DAVA::Signal<StyleSheetSelectorsSection*, StyleSheetSelectorProperty*, DAVA::int32> styleSelectorWillBeAdded;
    DAVA::Signal<StyleSheetSelectorsSection*, StyleSheetSelectorProperty*, DAVA::int32> styleSelectorWasAdded;
    DAVA::Signal<StyleSheetSelectorsSection*, StyleSheetSelectorProperty*, DAVA::int32> styleSelectorWillBeRemoved;
    DAVA::Signal<StyleSheetSelectorsSection*, StyleSheetSelectorProperty*, DAVA::int32> styleSelectorWasRemoved;

private:
    void UpdateStyleSheetPropertyTable();

private:
    enum eSection
    {
        SECTION_SELECTORS = 0,
        SECTION_PROPERTIES = 1,
        SECTION_COUNT = 2
    };

private:
    StyleSheetNode* styleSheet = nullptr;

    StyleSheetSelectorsSection* selectors = nullptr;
    StyleSheetPropertiesSection* propertiesSection = nullptr;

    DAVA::UIStyleSheetPropertyTable* propertyTable = nullptr;
};

#endif // __QUICKED_STYLE_SHEETS_ROOT_PROPERTY_H__
