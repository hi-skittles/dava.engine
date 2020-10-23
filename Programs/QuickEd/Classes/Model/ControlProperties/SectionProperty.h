#ifndef __QUICKED_SECTION_PROPERTY_H__
#define __QUICKED_SECTION_PROPERTY_H__

#include "AbstractProperty.h"

template <typename ValueType>
class SectionProperty : public AbstractProperty
{
public:
    SectionProperty(const DAVA::String& sectionName);

protected:
    virtual ~SectionProperty();

public:
    void AddProperty(ValueType* property);
    void InsertProperty(ValueType* property, DAVA::int32 index);
    void RemoveProperty(ValueType* property);
    DAVA::uint32 GetCount() const override;
    ValueType* GetProperty(DAVA::int32 index) const override;

    void Refresh(DAVA::int32 refreshFlags) override;
    void Accept(PropertyVisitor* visitor) override;

    const DAVA::String& GetName() const override;

    virtual ValueType* FindChildPropertyByName(const DAVA::String& name) const;

    ePropertyType GetType() const override
    {
        return TYPE_HEADER;
    }

    const DAVA::Type* GetValueType() const override
    {
        return nullptr;
    }

    typename DAVA::Vector<ValueType*>::const_iterator begin() const;
    typename DAVA::Vector<ValueType*>::const_iterator end() const;

    typename DAVA::Vector<ValueType*>::iterator begin();
    typename DAVA::Vector<ValueType*>::iterator end();

protected:
    DAVA::Vector<ValueType*> children;
    DAVA::String name;
};

template <typename ValueType>
inline SectionProperty<ValueType>::SectionProperty(const DAVA::String& sectionName)
    : name(sectionName)
{
}

template <typename ValueType>
inline SectionProperty<ValueType>::~SectionProperty()
{
    for (auto it = children.begin(); it != children.end(); ++it)
    {
        DVASSERT((*it)->GetParent() == this);
        (*it)->SetParent(nullptr);
        (*it)->Release();
    }
    children.clear();
}

template <typename ValueType>
inline void SectionProperty<ValueType>::AddProperty(ValueType* property)
{
    DVASSERT(property->GetParent() == nullptr);
    property->SetParent(this);
    children.push_back(SafeRetain(property));
}

template <typename ValueType>
inline void SectionProperty<ValueType>::InsertProperty(ValueType* property, DAVA::int32 index)
{
    DVASSERT(property->GetParent() == nullptr);
    if (0 <= index && index <= static_cast<DAVA::int32>(children.size()))
    {
        property->SetParent(this);
        children.insert(children.begin() + index, SafeRetain(property));
    }
    else
    {
        DVASSERT(false);
    }
}

template <typename ValueType>
inline void SectionProperty<ValueType>::RemoveProperty(ValueType* property)
{
    auto it = std::find(children.begin(), children.end(), property);
    if (it != children.end())
    {
        DVASSERT((*it)->GetParent() == this);
        (*it)->SetParent(nullptr);
        (*it)->Release();
        children.erase(it);
    }
    else
    {
        DVASSERT(false);
    }
}

template <typename ValueType>
inline DAVA::uint32 SectionProperty<ValueType>::GetCount() const
{
    return static_cast<DAVA::uint32>(children.size());
}

template <typename ValueType>
inline ValueType* SectionProperty<ValueType>::GetProperty(DAVA::int32 index) const
{
    if (0 <= index && index < static_cast<DAVA::int32>(children.size()))
        return children[index];

    DVASSERT(false);
    return nullptr;
}

template <typename ValueType>
inline void SectionProperty<ValueType>::Refresh(DAVA::int32 refreshFlags)
{
    for (ValueType* prop : children)
        prop->Refresh(refreshFlags);
}

template <typename ValueType>
inline void SectionProperty<ValueType>::Accept(PropertyVisitor* visitor)
{
    // do nothing
}

template <typename ValueType>
const DAVA::String& SectionProperty<ValueType>::GetName() const
{
    return name;
}

template <typename ValueType>
ValueType* SectionProperty<ValueType>::FindChildPropertyByName(const DAVA::String& name) const
{
    for (auto child : children)
    {
        if (child->GetName() == name)
            return child;
    }
    return nullptr;
}

template <typename ValueType>
typename DAVA::Vector<ValueType*>::const_iterator SectionProperty<ValueType>::begin() const
{
    return children.begin();
}

template <typename ValueType>
typename DAVA::Vector<ValueType*>::const_iterator SectionProperty<ValueType>::end() const
{
    return children.end();
}

template <typename ValueType>
typename DAVA::Vector<ValueType*>::iterator SectionProperty<ValueType>::begin()
{
    return children.begin();
}

template <typename ValueType>
typename DAVA::Vector<ValueType*>::iterator SectionProperty<ValueType>::end()
{
    return children.end();
}

#endif // __QUICKED_SECTION_PROPERTY_H__
