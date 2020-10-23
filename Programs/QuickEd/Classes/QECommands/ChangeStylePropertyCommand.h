#pragma once

#include "QECommands/Private/QEPackageCommand.h"
#include <Base/Any.h>

class StyleSheetNode;
class AbstractProperty;

class ChangeStylePropertyCommand : public QEPackageCommand
{
public:
    ChangeStylePropertyCommand(PackageNode* package, StyleSheetNode* node, AbstractProperty* property, const DAVA::Any& newValue);

    void Redo() override;
    void Undo() override;

private:
    DAVA::RefPtr<StyleSheetNode> node;
    DAVA::RefPtr<AbstractProperty> property;
    DAVA::Any oldValue;
    DAVA::Any newValue;
};
