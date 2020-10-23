#pragma once

#include "QECommands/Private/QEPackageCommand.h"

class StyleSheetNode;
class StyleSheetProperty;

class AddStylePropertyCommand : public QEPackageCommand
{
public:
    AddStylePropertyCommand(PackageNode* package, StyleSheetNode* node, StyleSheetProperty* property);

    void Redo() override;
    void Undo() override;

private:
    DAVA::RefPtr<StyleSheetNode> node;
    DAVA::RefPtr<StyleSheetProperty> property;
};
