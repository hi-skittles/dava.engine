#include "Command/Command.h"

#include <Reflection/ReflectionRegistrator.h>

namespace DAVA
{
Command::Command(const String& description_)
    : description(description_)
{
}

DAVA_VIRTUAL_REFLECTION_IMPL(Command)
{
    DAVA::ReflectionRegistrator<Command>::Begin()
    .End();
}
}
