#pragma once

#include "QECommands/Private/QEPackageCommand.h"

#include <Base/Any.h>

class ControlNode;
class AbstractProperty;

class ResizeCommand : public QEPackageCommand
{
public:
    ResizeCommand(PackageNode* package);
    void AddNodePropertyValue(ControlNode* node, AbstractProperty* sizeProperty, const DAVA::Any& sizeValue, AbstractProperty* pivotProperty, const DAVA::Any& pivotValue);

    void Redo() override;
    void Undo() override;

    bool MergeWith(const DAVA::Command* command) override;

private:
    struct Item
    {
        Item(ControlNode* node, AbstractProperty* sizeProperty, const DAVA::Any& sizeValue, AbstractProperty* pivotProperty, const DAVA::Any& pivotValue);
        DAVA::RefPtr<ControlNode> node;
        DAVA::RefPtr<AbstractProperty> sizeProperty;
        DAVA::Any sizeNewValue;
        DAVA::Any sizeOldValue;

        DAVA::RefPtr<AbstractProperty> pivotProperty;
        DAVA::Any pivotNewValue;
        DAVA::Any pivotOldValue;
    };
    DAVA::Vector<Item> items;
};
