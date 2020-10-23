#ifndef __QUICKED_STYLE_SHEET_PROPERTY_H__
#define __QUICKED_STYLE_SHEET_PROPERTY_H__

#include "Model/ControlProperties/ValueProperty.h"
#include "UI/Styles/UIStyleSheetStructs.h"

class ValueProperty;
class IntrospectionProperty;

namespace DAVA
{
class UIControl;
}

class StyleSheetProperty : public ValueProperty
{
public:
    StyleSheetProperty(const DAVA::UIStyleSheetProperty& aProperty);

protected:
    virtual ~StyleSheetProperty();

public:
    void Accept(PropertyVisitor* visitor) override;

    DAVA::uint32 GetFlags() const override;

    ePropertyType GetType() const override;
    const EnumMap* GetEnumMap() const override;
    DAVA::Any GetValue() const override;
    void ApplyValue(const DAVA::Any& value) override;

    DAVA::Interpolation::FuncType GetTransitionFunction() const;
    void SetTransitionFunction(DAVA::Interpolation::FuncType type);

    DAVA::float32 GetTransitionTime() const;
    void SetTransitionTime(DAVA::float32 transitionTime);

    bool HasTransition() const;
    void SetTransition(bool transition);

    DAVA::uint32 GetPropertyIndex() const;
    const DAVA::UIStyleSheetProperty& GetProperty() const;

private:
    DAVA::RefPtr<class UIStyleSheetPropertyWrapper> property;
};

#endif // __QUICKED_STYLE_SHEET_PROPERTY_H__
