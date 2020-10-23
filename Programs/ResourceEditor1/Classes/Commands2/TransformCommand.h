#pragma once

#include "Classes/Selection/Selectable.h"
#include "Commands2/Base/RECommand.h"

class TransformCommand : public RECommand
{
public:
    TransformCommand(Selectable object, const DAVA::Matrix4& origTransform, const DAVA::Matrix4& newTransform);

    void Undo() override;
    void Redo() override;

    DAVA::Entity* GetEntity() const;
    const Selectable& GetTransformedObject() const;

protected:
    Selectable object;
    DAVA::Matrix4 undoTransform;
    DAVA::Matrix4 redoTransform;
};
