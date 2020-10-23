#pragma once

#include "Base/BaseTypes.h"

class RECommandIDHandler
{
public:
    RECommandIDHandler(DAVA::uint32 id);

    DAVA::uint32 GetID() const;
    virtual bool MatchCommandID(DAVA::uint32 commandID) const;
    bool MatchCommandIDs(const DAVA::Vector<DAVA::uint32>& commandIDVector) const;

private:
    const DAVA::uint32 id;
};

inline DAVA::uint32 RECommandIDHandler::GetID() const
{
    return id;
}

inline bool RECommandIDHandler::MatchCommandID(DAVA::uint32 commandID) const
{
    return (id == commandID);
}
