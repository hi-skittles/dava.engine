#include "PrototypeNameProperty.h"

#include "PropertyVisitor.h"
#include "../PackageHierarchy/ControlNode.h"
#include "../PackageHierarchy/PackageNode.h"

using namespace DAVA;

PrototypeNameProperty::PrototypeNameProperty(ControlNode* aNode, const PrototypeNameProperty* prototypeProperty)
    : ValueProperty("Prototype", Type::Instance<String>())
    , node(aNode) // weak
{
}

PrototypeNameProperty::~PrototypeNameProperty()
{
    node = nullptr; // weak
}

void PrototypeNameProperty::Accept(PropertyVisitor* visitor)
{
    visitor->VisitPrototypeNameProperty(this);
}

AbstractProperty::ePropertyType PrototypeNameProperty::GetType() const
{
    return TYPE_VARIANT;
}

DAVA::Any PrototypeNameProperty::GetValue() const
{
    return Any(GetPrototypeName());
}

bool PrototypeNameProperty::IsReadOnly() const
{
    return true;
}

String PrototypeNameProperty::GetPrototypeName() const
{
    ControlNode* prototype = node->GetPrototype();
    if (prototype)
    {
        String path = "";
        if (node->GetCreationType() == ControlNode::CREATED_FROM_PROTOTYPE_CHILD)
        {
            path = String("/") + node->GetPathToPrototypeChild();
        }

        const PackageNode* package = prototype->GetPackage();
        if (package && package->IsImported())
        {
            return package->GetName() + "/" + prototype->GetName() + path;
        }
        else
        {
            return prototype->GetName() + path;
        }
    }

    return String("");
}

ControlNode* PrototypeNameProperty::GetControl() const
{
    return node;
}

void PrototypeNameProperty::ApplyValue(const DAVA::Any& value)
{
    // do nothing
}
