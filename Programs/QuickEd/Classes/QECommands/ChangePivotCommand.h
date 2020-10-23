#pragma once

#include "QECommands/Private/QEPackageCommand.h"

#include <Base/Any.h>

class ControlNode;
class AbstractProperty;

class ChangePivotCommand : public QEPackageCommand
{
public:
    ChangePivotCommand(PackageNode* package);
    void AddNodePropertyValue(ControlNode* node, AbstractProperty* pivotProperty, const DAVA::Any& pivotValue, AbstractProperty* positionProperty, const DAVA::Any& positionValue);

    void Redo() override;
    void Undo() override;

    bool MergeWith(const DAVA::Command* command) override;

private:
    struct Item
    {
        Item(ControlNode* node, AbstractProperty* sizeProperty, const DAVA::Any& sizeValue, AbstractProperty* pivotProperty, const DAVA::Any& pivotValue);
        DAVA::RefPtr<ControlNode> node;
        DAVA::RefPtr<AbstractProperty> pivotProperty;
        DAVA::Any pivotNewValue;
        DAVA::Any pivotOldValue;

        DAVA::RefPtr<AbstractProperty> positionProperty;
        DAVA::Any positionNewValue;
        DAVA::Any positionOldValue;
    };
    DAVA::Vector<Item> items;
};
