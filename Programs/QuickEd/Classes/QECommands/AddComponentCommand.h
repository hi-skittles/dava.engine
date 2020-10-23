#pragma once

#include "QECommands/Private/QEPackageCommand.h"

class ControlNode;
class ComponentPropertiesSection;

class AddComponentCommand : public QEPackageCommand
{
public:
    AddComponentCommand(PackageNode* package, ControlNode* node, ComponentPropertiesSection* section);

    void Redo() override;
    void Undo() override;

private:
    DAVA::RefPtr<ControlNode> node;
    DAVA::RefPtr<ComponentPropertiesSection> section;
};
