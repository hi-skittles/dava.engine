#pragma once

#include "Model/PackageHierarchy/PackageNode.h"
#include "QECommands/Private/QEPackageCommand.h"

#include <Math/Vector.h>

class PackageBaseNode;

class SetGuidesCommand : public QEPackageCommand
{
public:
    SetGuidesCommand(PackageNode* package, const DAVA::String& controlName, DAVA::Vector2::eAxis orientation, const PackageNode::AxisGuides& guides);

    void Redo() override;
    void Undo() override;

private:
    DAVA::String controlName;
    const DAVA::Vector2::eAxis orientation;
    PackageNode::AxisGuides guides;
    PackageNode::AxisGuides oldGuides;
};
