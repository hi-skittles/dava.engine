#include "Commands2/Base/RECommand.h"

RECommand::RECommand(DAVA::uint32 id, const DAVA::String& description_)
    : Command(description_)
    , RECommandIDHandler(id)
{
}
