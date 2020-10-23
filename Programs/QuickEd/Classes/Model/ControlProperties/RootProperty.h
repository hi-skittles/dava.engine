#ifndef __UI_EDITOR_ROOT_PROPERTY_H__
#define __UI_EDITOR_ROOT_PROPERTY_H__

#include "Model/ControlProperties/AbstractProperty.h"
#include <Functional/Signal.h>

class ControlPropertiesSection;
class ComponentPropertiesSection;
class BackgroundPropertiesSection;
class InternalControlPropertiesSection;
class ValueProperty;
class NameProperty;
class PrototypeNameProperty;
class ClassProperty;
class CustomClassProperty;
class VisibleValueProperty;
class ControlNode;

namespace DAVA
{
class UIControl;
}

class RootProperty : public AbstractProperty
{
public:
    RootProperty(ControlNode* node, const RootProperty* sourceProperties);

protected:
    virtual ~RootProperty();

public:
    virtual DAVA::uint32 GetCount() const override;
    virtual AbstractProperty* GetProperty(int index) const override;

    ClassProperty* GetClassProperty() const
    {
        return classProperty;
    }
    CustomClassProperty* GetCustomClassProperty() const
    {
        return customClassProperty;
    }
    PrototypeNameProperty* GetPrototypeProperty() const
    {
        return prototypeProperty;
    }
    NameProperty* GetNameProperty() const
    {
        return nameProperty;
    }
    VisibleValueProperty* GetVisibleProperty() const
    {
        return visibleProperty;
    }

    DAVA::int32 GetControlPropertiesSectionsCount() const;
    ControlPropertiesSection* GetControlPropertiesSection(DAVA::int32 index) const;
    ControlPropertiesSection* GetControlPropertiesSection(const DAVA::String& name) const;

    bool CanAddComponent(const DAVA::Type* componentType) const;
    bool CanRemoveComponent(const DAVA::Type* componentType, DAVA::uint32 index = 0) const;
    const DAVA::Vector<ComponentPropertiesSection*>& GetComponents() const;
    DAVA::int32 GetIndexOfCompoentPropertiesSection(ComponentPropertiesSection* section) const;
    ComponentPropertiesSection* FindComponentPropertiesSection(const DAVA::Type* componentType, DAVA::uint32 index) const;
    ComponentPropertiesSection* AddComponentPropertiesSection(const DAVA::Type* componentType);
    void AddComponentPropertiesSection(ComponentPropertiesSection* section);
    void RemoveComponentPropertiesSection(const DAVA::Type* componentType, DAVA::uint32 componentIndex);
    void RemoveComponentPropertiesSection(ComponentPropertiesSection* section);

    void AttachPrototypeComponent(ComponentPropertiesSection* section, ComponentPropertiesSection* prototypeSection);
    void DetachPrototypeComponent(ComponentPropertiesSection* section, ComponentPropertiesSection* prototypeSection);

    void SetProperty(AbstractProperty* property, const DAVA::Any& newValue);
    void SetBindingProperty(AbstractProperty* property, const DAVA::String& newValue, DAVA::int32 bindingUpdateMode);
    void SetDefaultProperty(AbstractProperty* property, const DAVA::Any& newValue);
    void ResetProperty(AbstractProperty* property);
    void SetPropertyForceOverride(ValueProperty* property, bool forceOverride);
    void RefreshProperty(AbstractProperty* property, DAVA::int32 refreshFlags);

    void Refresh(DAVA::int32 refreshFlags) override;
    void Accept(PropertyVisitor* visitor) override;
    bool IsReadOnly() const override;

    const DAVA::String& GetName() const override;
    const DAVA::Type* GetValueType() const override;
    ePropertyType GetType() const override;

    ControlNode* GetControlNode() const;

    DAVA::Signal<AbstractProperty*> propertyChanged;
    DAVA::Signal<RootProperty*, ComponentPropertiesSection*, DAVA::int32> componentPropertiesWillBeAdded;
    DAVA::Signal<RootProperty*, ComponentPropertiesSection*, DAVA::int32> componentPropertiesWasAdded;
    DAVA::Signal<RootProperty*, ComponentPropertiesSection*, DAVA::int32> componentPropertiesWillBeRemoved;
    DAVA::Signal<RootProperty*, ComponentPropertiesSection*, DAVA::int32> componentPropertiesWasRemoved;

private:
    void AddBaseProperties(DAVA::UIControl* control, const RootProperty* sourceProperties);
    void MakeControlPropertiesSection(DAVA::UIControl* control, const DAVA::Type* type, const DAVA::Vector<DAVA::Reflection::Field>& fields, const RootProperty* sourceProperties);
    DAVA::uint32 GetComponentAbsIndex(const DAVA::Type* componentType, DAVA::uint32 index) const;
    void RefreshComponentIndices();

private:
    ControlNode* node = nullptr; // weak ref

    ClassProperty* classProperty = nullptr;
    CustomClassProperty* customClassProperty = nullptr;
    PrototypeNameProperty* prototypeProperty = nullptr;
    NameProperty* nameProperty = nullptr;
    VisibleValueProperty* visibleProperty = nullptr; //weak ptr

    DAVA::Vector<ValueProperty*> baseProperties;
    DAVA::Vector<ControlPropertiesSection*> controlProperties;
    DAVA::Vector<ComponentPropertiesSection*> componentProperties;
};

#endif // __UI_EDITOR_ROOT_PROPERTY_H__
