#ifndef __ENTITY_PARENT_CHANGE_COMMAND_H__
#define __ENTITY_PARENT_CHANGE_COMMAND_H__

#include "Commands2/Base/RECommand.h"

namespace DAVA
{
class Entity;
}

class EntityParentChangeCommand : public RECommand
{
public:
    EntityParentChangeCommand(DAVA::Entity* entity, DAVA::Entity* newParent, DAVA::Entity* newBefore = NULL);
    ~EntityParentChangeCommand();

    void Undo() override;
    void Redo() override;
    DAVA::Entity* GetEntity() const;

    DAVA::Entity* entity;
    DAVA::Entity* oldParent;
    DAVA::Entity* oldBefore;
    DAVA::Entity* newParent;
    DAVA::Entity* newBefore;
};

#endif // __ENTITY_PARENT_CHANGE_COMMAND_H__
