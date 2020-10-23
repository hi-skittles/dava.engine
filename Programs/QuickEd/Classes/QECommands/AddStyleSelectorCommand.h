
#pragma once

#include "QECommands/Private/QEPackageCommand.h"

class StyleSheetNode;
class StyleSheetSelectorProperty;

class AddStyleSelectorCommand : public QEPackageCommand
{
public:
    AddStyleSelectorCommand(PackageNode* package, StyleSheetNode* node, StyleSheetSelectorProperty* property);

    void Redo() override;
    void Undo() override;

private:
    DAVA::RefPtr<StyleSheetNode> node;
    DAVA::RefPtr<StyleSheetSelectorProperty> property;
    int index = -1;
};
