#pragma once

#include "Classes/Model/PackageHierarchy/PackageBaseNode.h"

class ControlNode;

class ControlsContainerNode : public PackageBaseNode
{
public:
    ControlsContainerNode(PackageBaseNode* parent);

    virtual DAVA::Vector<ControlNode*>::const_iterator begin() const;
    virtual DAVA::Vector<ControlNode*>::const_iterator end() const;

    virtual DAVA::Vector<ControlNode*>::iterator begin();
    virtual DAVA::Vector<ControlNode*>::iterator end();

    virtual void Add(ControlNode* node) = 0;
    virtual void InsertAtIndex(int index, ControlNode* node) = 0;
    virtual void Remove(ControlNode* node) = 0;

    ControlNode* FindChildByName(const DAVA::String& name) const;

protected:
    ~ControlsContainerNode() override = default;
    DAVA::Vector<ControlNode*> nodes;
};
