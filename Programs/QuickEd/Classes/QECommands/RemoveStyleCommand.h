#pragma once

#include "QECommands/Private/QEPackageCommand.h"

class StyleSheetNode;
class StyleSheetsNode;

class RemoveStyleCommand : public QEPackageCommand
{
public:
    RemoveStyleCommand(PackageNode* package, StyleSheetNode* node, StyleSheetsNode* dest, int index);

    void Redo() override;
    void Undo() override;

private:
    DAVA::RefPtr<StyleSheetNode> node;
    DAVA::RefPtr<StyleSheetsNode> dest;
    const int index;
};
