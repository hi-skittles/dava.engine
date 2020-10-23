#pragma once

#include "QECommands/Private/QEPackageCommand.h"

class ControlNode;
class ControlsContainerNode;

class InsertControlCommand : public QEPackageCommand
{
public:
    InsertControlCommand(PackageNode* package, ControlNode* node, ControlsContainerNode* dest, int index);

    void Redo() override;
    void Undo() override;

private:
    DAVA::RefPtr<ControlNode> node;
    DAVA::RefPtr<ControlsContainerNode> dest;
    const int index;
};
