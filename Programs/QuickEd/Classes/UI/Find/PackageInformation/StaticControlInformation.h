#pragma once

#include "UI/Find/PackageInformation/ControlInformation.h"

#include <Base/Result.h>

class StaticPackageInformation;

class StaticControlInformation
: public ControlInformation
{
public:
    StaticControlInformation(const DAVA::FastName& name);
    StaticControlInformation(const StaticControlInformation& prototype, const DAVA::FastName& name, const std::shared_ptr<StaticPackageInformation> prototypePackage, const DAVA::FastName& prototypeName);

    DAVA::FastName GetName() const override;
    DAVA::FastName GetPrototypeName() const override;
    DAVA::String GetPrototypePackagePath() const override;
    const ControlInformation* GetPrototype() const override;
    bool HasErrors() const override;

    bool HasComponent(const DAVA::Type* componentType) const override;

    void VisitParent(const DAVA::Function<void(const ControlInformation*)>& visitor) const override;
    void VisitChildren(const DAVA::Function<void(const ControlInformation*)>& visitor) const override;

    // for now returns a value ONLY if it is set explicitly
    DAVA::Any GetControlPropertyValue(const DAVA::ReflectedStructure::Field& member) const override;
    DAVA::Any GetComponentPropertyValue(const DAVA::Type* componentTypeconst, DAVA::int32 componentIndex, const DAVA::ReflectedStructure::Field& member) const override;

    StaticControlInformation* GetParent() const;
    void SetParent(StaticControlInformation* parent);

    void AddChild(const std::shared_ptr<StaticControlInformation>& child);
    const DAVA::Vector<std::shared_ptr<StaticControlInformation>>& GetChildren() const;
    std::shared_ptr<StaticControlInformation> FindChildByName(const DAVA::FastName& name) const;

    void AddComponent(const DAVA::Type* componentType);

    void SetControlProperty(const DAVA::ReflectedStructure::Field& member, const DAVA::Any& value);
    void SetComponentProperty(const DAVA::Type* componentType, DAVA::int32 componentIndex, const DAVA::ReflectedStructure::Field& member, const DAVA::Any& value);

    void AddResult(const DAVA::Result& result);

private:
    using ComponentPropertyId = std::tuple<const DAVA::Type*, DAVA::int32, DAVA::FastName>;

    void InitFrom(const StaticControlInformation& other);

    DAVA::FastName name;
    StaticControlInformation* parent = nullptr;

    std::shared_ptr<StaticPackageInformation> prototypePackage;
    DAVA::FastName prototypeName;

    DAVA::UnorderedMap<const DAVA::Type*, DAVA::int32> componentCount;

    DAVA::Map<DAVA::FastName, DAVA::Any> controlProperties;
    DAVA::Map<ComponentPropertyId, DAVA::Any> componentProperties;

    DAVA::Vector<std::shared_ptr<StaticControlInformation>> children;

    DAVA::ResultList results;
    std::shared_ptr<StaticControlInformation> prototype;
};
