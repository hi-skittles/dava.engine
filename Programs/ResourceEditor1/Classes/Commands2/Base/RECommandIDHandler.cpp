#include "Commands2/Base/RECommandIDHandler.h"

RECommandIDHandler::RECommandIDHandler(DAVA::uint32 id_)
    : id(id_)
{
}

bool RECommandIDHandler::MatchCommandIDs(const DAVA::Vector<DAVA::uint32>& commandIDVector) const
{
    auto functor = [this](DAVA::uint32 id) { return MatchCommandID(id); };
    return std::find_if(commandIDVector.begin(), commandIDVector.end(), functor) != commandIDVector.end();
}
