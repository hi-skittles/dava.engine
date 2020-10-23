#pragma once

#include "QECommands/Private/QEPackageCommand.h"
#include <Base/Any.h>

class ControlNode;
class ValueProperty;

class ChangePropertyForceOverrideCommand : public QEPackageCommand
{
public:
    ChangePropertyForceOverrideCommand(PackageNode* package, ControlNode* node, ValueProperty* property);

    void Redo() override;
    void Undo() override;

private:
    DAVA::RefPtr<ControlNode> node;
    DAVA::RefPtr<ValueProperty> property;
    bool prevForceOverriden = false;
};
