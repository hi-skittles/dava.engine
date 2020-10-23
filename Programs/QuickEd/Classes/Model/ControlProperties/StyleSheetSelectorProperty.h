#ifndef __QUICKED_STYLE_SHEET_SELECTOR_PROPERTY_H__
#define __QUICKED_STYLE_SHEET_SELECTOR_PROPERTY_H__

#include "Model/ControlProperties/ValueProperty.h"
#include "UI/Styles/UIStyleSheetSelectorChain.h"

class ValueProperty;

class StyleSheetNode;

namespace DAVA
{
class UIControl;
class UIStyleSheet;
struct UIStyleSheetSourceInfo;
class UIStyleSheetPropertyTable;
}

class StyleSheetSelectorProperty : public ValueProperty
{
public:
    StyleSheetSelectorProperty(const DAVA::UIStyleSheetSelectorChain& chain, const DAVA::UIStyleSheetSourceInfo& sourceInfo);

protected:
    virtual ~StyleSheetSelectorProperty();

public:
    DAVA::uint32 GetCount() const override;
    AbstractProperty* GetProperty(int index) const override;

    void Accept(PropertyVisitor* visitor) override;

    ePropertyType GetType() const override;
    DAVA::uint32 GetFlags() const override;

    DAVA::Any GetValue() const override;
    void ApplyValue(const DAVA::Any& value) override;

    const DAVA::UIStyleSheetSelectorChain& GetSelectorChain() const;
    const DAVA::String& GetSelectorChainString() const;
    DAVA::UIStyleSheet* GetStyleSheet() const;

    void SetStyleSheetPropertyTable(DAVA::UIStyleSheetPropertyTable* propertyTable);

private:
    DAVA::UIStyleSheet* styleSheet = nullptr;
    DAVA::String value;
};

#endif // __QUICKED_STYLE_SHEET_SELECTOR_PROPERTY_H__
