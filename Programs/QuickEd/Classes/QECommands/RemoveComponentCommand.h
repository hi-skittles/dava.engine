#pragma once

#include "QECommands/Private/QEPackageCommand.h"

class ControlNode;
class ComponentPropertiesSection;

class RemoveComponentCommand : public QEPackageCommand
{
public:
    RemoveComponentCommand(PackageNode* package, ControlNode* node, ComponentPropertiesSection* section);

    void Redo() override;
    void Undo() override;

private:
    DAVA::RefPtr<ControlNode> node;
    DAVA::RefPtr<ComponentPropertiesSection> componentSection;
};
