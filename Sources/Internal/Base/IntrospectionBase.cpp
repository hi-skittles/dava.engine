#include "Base/IntrospectionBase.h"
#include "Base/Meta.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(InspBase)
{
    ReflectionRegistrator<InspBase>::Begin()
    .End();
}

InspBase::InspBase() = default;
InspBase::~InspBase() = default;

InspMember::InspMember(const char* _name, const InspDesc& _desc, const size_t _offset, const MetaInfo* _type, int _flags /* = 0 */)
    : name(_name)
    , desc(_desc)
    , offset(_offset)
    , type(_type)
    , flags(_flags)
    , parentInsp(nullptr)
{
}

const FastName& InspMember::Name() const
{
    return name;
}

const InspDesc& InspMember::Desc() const
{
    return desc;
}

const MetaInfo* InspMember::Type() const
{
    return type;
}

void* InspMember::Pointer(void* object) const
{
    return OffsetPointer<void>(object, offset);
}

void* InspMember::Data(void* object) const
{
    if (type->IsPointer())
    {
        return *static_cast<void**>(Pointer(object));
    }
    else
    {
        return Pointer(object);
    }
}

VariantType::eVariantType InspMember::ValueType() const
{
    return VariantType::TypeFromMetaInfo(type);
}

VariantType InspMember::Value(void* object) const
{
    return VariantType::LoadData(Pointer(object), type);
}

void InspMember::SetValue(void* object, const VariantType& val) const
{
    VariantType::SaveData(Pointer(object), type, val);
}

void InspMember::SetValueRaw(void* object, void* val) const
{
    DVASSERT(false);
}

const InspColl* InspMember::Collection() const
{
    return nullptr;
}

const InspMemberDynamic* InspMember::Dynamic() const
{
    return nullptr;
}

const InspInfo* InspMember::GetParentInsp() const
{
    return parentInsp;
}

int InspMember::Flags() const
{
    return flags;
}

void InspMember::ApplyParentInsp(const InspInfo* _parentInsp) const
{
    parentInsp = _parentInsp;
}

InspColl::InspColl(const char* _name, const InspDesc& _desc, const size_t _offset, const MetaInfo* _type, int _flags)
    : InspMember(_name, _desc, _offset, _type, _flags)
{
}
}
