#pragma once

#include "Commands2/Base/RECommand.h"

namespace DAVA
{
class RenderObject;
class RenderSystem;
class BillboardRenderObject;
class RenderComponent;
class Component;
class Entity;
}

class ConvertToBillboardCommand : public RECommand
{
public:
    ConvertToBillboardCommand(DAVA::RenderObject*, DAVA::Entity*);
    ~ConvertToBillboardCommand();

    void Undo() override;
    void Redo() override;

    DAVA::Entity* GetEntity() const;

private:
    DAVA::Entity* entity = nullptr;
    DAVA::RenderComponent* oldRenderComponent = nullptr;
    DAVA::RenderComponent* newRenderComponent = nullptr;
    DAVA::Component* detachedComponent = nullptr;
};

inline DAVA::Entity* ConvertToBillboardCommand::GetEntity() const
{
    return entity;
}
