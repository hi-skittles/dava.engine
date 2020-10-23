#include "Classes/Model/PackageHierarchy/ControlsContainerNode.h"

#include "Classes/Model/PackageHierarchy/ControlNode.h"

ControlsContainerNode::ControlsContainerNode(PackageBaseNode* parent)
    : PackageBaseNode(parent)
{
}

ControlNode* ControlsContainerNode::FindChildByName(const DAVA::String& name) const
{
    for (ControlNode* child : nodes)
    {
        if (child->GetName() == name)
        {
            return child;
        }
    }
    return nullptr;
}

DAVA::Vector<ControlNode*>::const_iterator ControlsContainerNode::begin() const
{
    return nodes.begin();
}

DAVA::Vector<ControlNode*>::const_iterator ControlsContainerNode::end() const
{
    return nodes.end();
}

DAVA::Vector<ControlNode*>::iterator ControlsContainerNode::begin()
{
    return nodes.begin();
}

DAVA::Vector<ControlNode*>::iterator ControlsContainerNode::end()
{
    return nodes.end();
}
