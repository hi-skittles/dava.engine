#pragma once

#include "QECommands/Private/QEPackageCommand.h"

class ControlNode;
class ComponentPropertiesSection;

class AttachComponentPrototypeSectionCommand : public QEPackageCommand
{
public:
    AttachComponentPrototypeSectionCommand(PackageNode* package, ControlNode* node, ComponentPropertiesSection* destSection, ComponentPropertiesSection* prototypeSection);

    void Redo() override;
    void Undo() override;

private:
    DAVA::RefPtr<ControlNode> node;
    DAVA::RefPtr<ComponentPropertiesSection> destSection;
    DAVA::RefPtr<ComponentPropertiesSection> prototypeSection;
};
