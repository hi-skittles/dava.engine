#pragma once

#include "FileSystem/VariantType.h"
#include "EditorSystems/EditorSystemsManager.h"

#include "Command/Command.h"

class PackageNode;
class ControlNode;
class AbstractProperty;

class ChangeBindingCommand : public DAVA::Command
{
public:
    ChangeBindingCommand(PackageNode* root, ControlNode* node, AbstractProperty* property, const DAVA::String& newVal, DAVA::int32 newMode);
    ~ChangeBindingCommand() override = default;

    void Redo() override;
    void Undo() override;

private:
    DAVA::String GetValueFromProperty(AbstractProperty* property);
    PackageNode* root = nullptr;
    ControlNode* node = nullptr;
    AbstractProperty* property = nullptr;
    DAVA::String oldBindingValue;
    DAVA::String newBindingValue;

    DAVA::int32 oldMode = 0;
    DAVA::int32 newMode = 0;

    DAVA::Any oldValue;

    bool wasBound = false;
};
