#include "UI/Find/PackageInformation/ControlNodeInformation.h"
#include "Model/PackageHierarchy/PackageNode.h"
#include <UI/UIControl.h>

using namespace DAVA;

ControlNodeInformation::ControlNodeInformation(const ControlNode* controlNode_)
    : controlNode(controlNode_)
{
    const ControlNode* prototype = controlNode->GetPrototype();
    if (prototype != nullptr)
    {
        prototypeInformation.reset(new ControlNodeInformation(prototype));
    }
}

FastName ControlNodeInformation::GetName() const
{
    return FastName(controlNode->GetName());
}

FastName ControlNodeInformation::GetPrototypeName() const
{
    const ControlNode* prototype = controlNode->GetPrototype();

    if (prototype != nullptr)
    {
        return FastName(prototype->GetName());
    }
    else
    {
        return FastName();
    }
}

String ControlNodeInformation::GetPrototypePackagePath() const
{
    const ControlNode* prototype = controlNode->GetPrototype();

    if (prototype != nullptr)
    {
        return prototype->GetPackage()->GetPath().GetFrameworkPath();
    }
    else
    {
        return String();
    }
}

const ControlInformation* ControlNodeInformation::GetPrototype() const
{
    return prototypeInformation.get();
}

bool ControlNodeInformation::HasErrors() const
{
    return controlNode->HasErrors();
}

bool ControlNodeInformation::HasComponent(const DAVA::Type* componentType) const
{
    return controlNode->GetControl()->GetComponentCount(componentType) > 0;
}

void ControlNodeInformation::VisitParent(const Function<void(const ControlInformation*)>& visitor) const
{
    if (ControlNode* parentControlNode = dynamic_cast<ControlNode*>(controlNode->GetParent()))
    {
        ControlNodeInformation parentInfo(parentControlNode);

        visitor(&parentInfo);
    }
    else
    {
        visitor(nullptr);
    }
}

void ControlNodeInformation::VisitChildren(const Function<void(const ControlInformation*)>& visitor) const
{
    for (int32 childIndex = 0; childIndex < controlNode->GetCount(); ++childIndex)
    {
        if (ControlNode* childControlNode = dynamic_cast<ControlNode*>(controlNode->Get(childIndex)))
        {
            ControlNodeInformation childInfo(childControlNode);

            visitor(&childInfo);
        }
    }
}

Any ControlNodeInformation::GetControlPropertyValue(const DAVA::ReflectedStructure::Field& member) const
{
    return member.valueWrapper->GetValue(ReflectedObject(controlNode->GetControl()));
}

Any ControlNodeInformation::GetComponentPropertyValue(const DAVA::Type* componentType, DAVA::int32 componentIndex, const DAVA::ReflectedStructure::Field& member) const
{
    return member.valueWrapper->GetValue(ReflectedObject(controlNode->GetControl()->GetComponent(componentType, componentIndex)));
}
