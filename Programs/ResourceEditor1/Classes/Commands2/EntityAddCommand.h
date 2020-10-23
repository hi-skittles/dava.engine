#ifndef __ENTITY_ADD_COMMAND_H__
#define __ENTITY_ADD_COMMAND_H__

#include "Commands2/Base/RECommand.h"

namespace DAVA
{
class Entity;
}

class EntityAddCommand : public RECommand
{
public:
    EntityAddCommand(DAVA::Entity* entityToAdd, DAVA::Entity* toParent);
    ~EntityAddCommand();

    void Undo() override;
    void Redo() override;

    DAVA::Entity* GetEntity() const;

    DAVA::Entity* entityToAdd;
    DAVA::Entity* parentToAdd;
};

#endif // __ENTITY_ADD_COMMAND_H__
