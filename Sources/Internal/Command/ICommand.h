#pragma once

#include <Reflection/Reflection.h>

namespace DAVA
{
class ICommand : public ReflectionBase
{
public:
    virtual ~ICommand() = default;
    virtual void Redo() = 0;
    virtual void Undo() = 0;
};
}
