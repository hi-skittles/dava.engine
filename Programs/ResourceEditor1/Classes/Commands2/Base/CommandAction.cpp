#include "Commands2/Base/CommandAction.h"
#include "Debug/DVAssert.h"

CommandAction::CommandAction(DAVA::uint32 id, const DAVA::String& text)
    : RECommand(id, text)
{
}

void CommandAction::Undo()
{
    DVASSERT(false);
}

bool IsCommandAction(const DAVA::Command* command)
{
    return dynamic_cast<const CommandAction*>(command) != nullptr;
}
