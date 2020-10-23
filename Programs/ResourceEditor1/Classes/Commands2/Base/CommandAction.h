#pragma once

#include "Base/BaseTypes.h"
#include "Commands2/Base/RECommand.h"

class CommandAction : public RECommand
{
public:
    CommandAction(DAVA::uint32 id, const DAVA::String& text = DAVA::String());
    void Undo();
};

bool IsCommandAction(const DAVA::Command* command);
