#ifndef __ADD_COMPONENT_COMMAND_H__
#define __ADD_COMPONENT_COMMAND_H__

#include "Commands2/Base/RECommand.h"

namespace DAVA
{
class Entity;
class Component;
}

class AddComponentCommand : public RECommand
{
public:
    AddComponentCommand(DAVA::Entity* entity, DAVA::Component* component);
    ~AddComponentCommand() override;

    void Undo() override;
    void Redo() override;

    DAVA::Entity* GetEntity() const;
    DAVA::Component* GetComponent() const;

private:
    DAVA::Entity* entity = nullptr;
    DAVA::Component* component = nullptr;
    DAVA::Component* backup = nullptr;
};

#endif // __ADD_COMPONENT_COMMAND_H__
