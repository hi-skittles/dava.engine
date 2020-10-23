#ifndef __QUICKED_COMPONENT_PROPERTIES_SECTION_H__
#define __QUICKED_COMPONENT_PROPERTIES_SECTION_H__

#include "SectionProperty.h"
#include "IntrospectionProperty.h"

#include <UI/Components/UIComponent.h>

namespace DAVA
{
class UIControl;
}

class ComponentPropertiesSection : public SectionProperty<IntrospectionProperty>
{
public:
    ComponentPropertiesSection(DAVA::UIControl* control, const DAVA::Type* type, DAVA::int32 index, const ComponentPropertiesSection* prototypeSection);

protected:
    virtual ~ComponentPropertiesSection();

public:
    DAVA::UIComponent* GetComponent() const;
    const DAVA::Type* GetComponentType() const;

    const DAVA::String& GetDisplayName() const override;

    void AttachPrototypeSection(ComponentPropertiesSection* section);
    void DetachPrototypeSection(ComponentPropertiesSection* section);

    bool HasChanges() const override;
    DAVA::uint32 GetFlags() const override;

    void InstallComponent();
    void UninstallComponent();

    DAVA::int32 GetComponentIndex() const;
    void RefreshIndex();

    void Accept(PropertyVisitor* visitor) override;

    DAVA::String GetComponentName() const;

private:
    void RefreshName();

private:
    DAVA::UIControl* control;
    DAVA::UIComponent* component;
    DAVA::int32 index;
    const ComponentPropertiesSection* prototypeSection;

    bool componentWasCreated = false;
    DAVA::String displayName;
};

#endif // __QUICKED_COMPONENT_PROPERTIES_SECTION_H__
