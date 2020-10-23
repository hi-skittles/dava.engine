#pragma once

#include "Classes/Commands2/Base/RECommand.h"
#include "Reflection/Reflection.h"

#include "Base/Any.h"

class SetFieldValueCommand : public RECommand
{
public:
    SetFieldValueCommand(const DAVA::Reflection::Field& field, const DAVA::Any& newValue);

    void Redo() override;
    void Undo() override;

    const DAVA::Any& GetOldValue() const;
    const DAVA::Any& GetNewValue() const;
    const DAVA::Reflection::Field& GetField() const;

private:
    DAVA::Any oldValue;
    DAVA::Any newValue;
    DAVA::Reflection::Field field;
};
