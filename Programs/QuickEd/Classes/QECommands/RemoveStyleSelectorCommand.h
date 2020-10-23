
#pragma once

#include "QECommands/Private/QEPackageCommand.h"

class StyleSheetNode;
class StyleSheetSelectorProperty;

class RemoveStyleSelectorCommand : public QEPackageCommand
{
public:
    RemoveStyleSelectorCommand(PackageNode* package, StyleSheetNode* node, StyleSheetSelectorProperty* property);

    void Redo() override;
    void Undo() override;

private:
    DAVA::RefPtr<StyleSheetNode> node;
    DAVA::RefPtr<StyleSheetSelectorProperty> property;
    int index = -1;
};
