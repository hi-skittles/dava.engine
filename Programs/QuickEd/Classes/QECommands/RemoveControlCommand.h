#pragma once

#include "QECommands/Private/QEPackageCommand.h"

class ControlNode;
class ControlsContainerNode;

class RemoveControlCommand : public QEPackageCommand
{
public:
    RemoveControlCommand(PackageNode* package, ControlNode* node, ControlsContainerNode* from, int index);

    void Redo() override;
    void Undo() override;

private:
    DAVA::RefPtr<ControlNode> node;
    DAVA::RefPtr<ControlsContainerNode> from;
    const int index;
};
