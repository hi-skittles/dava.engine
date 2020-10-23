#pragma once

#include "Command/Command.h"
#include "Commands2/Base/RECommandIDHandler.h"

class RECommand : public DAVA::Command, public RECommandIDHandler
{
public:
    RECommand(DAVA::uint32 id, const DAVA::String& description = "");

    //re implement pure virtual function Undo for commands which can not make Undo itself
    void Undo() override
    {
    }
};
