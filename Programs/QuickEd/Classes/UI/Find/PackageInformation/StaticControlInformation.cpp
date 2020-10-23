#include "UI/Find/PackageInformation/StaticControlInformation.h"
#include "UI/Find/PackageInformation/StaticPackageInformation.h"

using namespace DAVA;

StaticControlInformation::StaticControlInformation(const FastName& name_)
    : name(name_)
{
}

StaticControlInformation::StaticControlInformation(const StaticControlInformation& prototype_, const FastName& name_, const std::shared_ptr<StaticPackageInformation> prototypePackage_, const FastName& prototypeName_)
    : name(name_)
    , prototypePackage(prototypePackage_)
    , prototypeName(prototypeName_)
    , prototype(new StaticControlInformation(prototype_))
{
    InitFrom(prototype_);
}

void StaticControlInformation::InitFrom(const StaticControlInformation& other)
{
    componentCount = other.componentCount;
    controlProperties = other.controlProperties;
    componentProperties = other.componentProperties;

    for (const std::shared_ptr<StaticControlInformation>& otherChild : other.children)
    {
        std::shared_ptr<StaticControlInformation> child = std::make_shared<StaticControlInformation>(otherChild->GetName());
        child->InitFrom(*otherChild);
        child->SetParent(this);
        children.push_back(child);
    }
}

FastName StaticControlInformation::GetName() const
{
    return name;
}

FastName StaticControlInformation::GetPrototypeName() const
{
    return prototypeName;
}

String StaticControlInformation::GetPrototypePackagePath() const
{
    return prototypePackage->GetPath();
}

const ControlInformation* StaticControlInformation::GetPrototype() const
{
    return prototype.get();
}

bool StaticControlInformation::HasErrors() const
{
    return results.HasErrors();
}

bool StaticControlInformation::HasComponent(const DAVA::Type* componentType) const
{
    const UnorderedMap<const DAVA::Type*, int32>::const_iterator iter = componentCount.find(componentType);

    if (iter != componentCount.end())
    {
        return iter->second > 0;
    }
    else
    {
        return false;
    }
}

void StaticControlInformation::VisitParent(const Function<void(const ControlInformation*)>& visitor) const
{
    visitor(parent);
}

void StaticControlInformation::VisitChildren(const Function<void(const ControlInformation*)>& visitor) const
{
    for (const std::shared_ptr<ControlInformation>& child : children)
    {
        visitor(child.get());
    }
}

Any StaticControlInformation::GetControlPropertyValue(const DAVA::ReflectedStructure::Field& member) const
{
    Map<FastName, Any>::const_iterator iter = controlProperties.find(FastName(member.name));

    if (iter != controlProperties.end())
    {
        return iter->second;
    }
    else
    {
        return Any();
    }
}

Any StaticControlInformation::GetComponentPropertyValue(const DAVA::Type* componentType, DAVA::int32 componentIndex, const DAVA::ReflectedStructure::Field& member) const
{
    const ComponentPropertyId id = std::make_tuple(componentType, componentIndex, FastName(member.name));
    auto iter = componentProperties.find(id);

    if (iter != componentProperties.end())
    {
        return iter->second;
    }
    else
    {
        return Any();
    }
}

StaticControlInformation* StaticControlInformation::GetParent() const
{
    return parent;
}

void StaticControlInformation::SetParent(StaticControlInformation* parent_)
{
    parent = parent_;
}

void StaticControlInformation::AddChild(const std::shared_ptr<StaticControlInformation>& child)
{
    children.push_back(child);
}

const Vector<std::shared_ptr<StaticControlInformation>>& StaticControlInformation::GetChildren() const
{
    return children;
}

std::shared_ptr<StaticControlInformation> StaticControlInformation::FindChildByName(const FastName& name) const
{
    for (const std::shared_ptr<StaticControlInformation>& c : children)
    {
        if (c->GetName() == name)
        {
            return c;
        }
    }

    return std::shared_ptr<StaticControlInformation>();
}

void StaticControlInformation::AddComponent(const DAVA::Type* componentType)
{
    ++componentCount[componentType];
}

void StaticControlInformation::SetControlProperty(const DAVA::ReflectedStructure::Field& member, const DAVA::Any& value)
{
    controlProperties[FastName(member.name)] = value;
}

void StaticControlInformation::SetComponentProperty(const DAVA::Type* componentType, int32 componentIndex, const DAVA::ReflectedStructure::Field& member, const DAVA::Any& value)
{
    const ComponentPropertyId id = std::make_tuple(componentType, componentIndex, FastName(member.name));
    componentProperties[id] = value;
}

void StaticControlInformation::AddResult(const DAVA::Result& result)
{
    results.AddResult(result);
}
