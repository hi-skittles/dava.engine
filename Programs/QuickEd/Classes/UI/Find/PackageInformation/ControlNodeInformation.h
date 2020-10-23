#pragma once

#include "Model/PackageHierarchy/ControlNode.h"
#include "UI/Find/PackageInformation/ControlInformation.h"

class ControlNodeInformation
: public ControlInformation
{
public:
    ControlNodeInformation(const ControlNode* controlNode);

    DAVA::FastName GetName() const override;
    DAVA::FastName GetPrototypeName() const override;
    DAVA::String GetPrototypePackagePath() const override;
    const ControlInformation* GetPrototype() const override;
    bool HasErrors() const override;

    bool HasComponent(const DAVA::Type* componentType) const override;

    void VisitParent(const DAVA::Function<void(const ControlInformation*)>& visitor) const override;
    void VisitChildren(const DAVA::Function<void(const ControlInformation*)>& visitor) const override;

    DAVA::Any GetControlPropertyValue(const DAVA::ReflectedStructure::Field& member) const override;
    DAVA::Any GetComponentPropertyValue(const DAVA::Type* componentType, DAVA::int32 componentIndex, const DAVA::ReflectedStructure::Field& member) const override;

private:
    const ControlNode* controlNode;
    std::unique_ptr<ControlNodeInformation> prototypeInformation;
};
