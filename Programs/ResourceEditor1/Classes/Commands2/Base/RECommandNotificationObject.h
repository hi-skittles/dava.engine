#pragma once

#include "Classes/Commands2/Base/RECommand.h"
#include "Classes/Commands2/Base/RECommandBatch.h"

#include <Functional/Function.h>

class RECommandNotificationObject
{
public:
    bool IsEmpty() const;
    void ExecuteForAllCommands(const DAVA::Function<void(const RECommand*, bool)>& fn) const;

    bool MatchCommandID(DAVA::uint32 commandID) const;
    bool MatchCommandIDs(const DAVA::Vector<DAVA::uint32>& commandIDVector) const;
    void ForEach(const DAVA::Function<void(const RECommand*)>& callback, DAVA::uint32 commandId) const;

    template <typename T>
    void ForEachWithCast(DAVA::uint32 commandId, const DAVA::Function<void(const T*)>& callback) const;

    const RECommand* command = nullptr;
    const RECommandBatch* batch = nullptr;
    bool redo = true;
};

template <typename T>
void RECommandNotificationObject::ForEachWithCast(DAVA::uint32 commandId, const DAVA::Function<void(const T*)>& callback) const
{
    static_assert(std::is_base_of<RECommand, T>::value, "Cast target should be derived from RECommand");

    auto fn = [callback](const RECommand* command) {
        callback(static_cast<const T*>(command));
    };
    ForEach(fn, commandId);
}

class REDependentCommandsHolder
{
    friend class RECommandStack;

public:
    REDependentCommandsHolder(const RECommandNotificationObject& notifyObject);

    void AddPreCommand(std::unique_ptr<DAVA::Command>&& command);
    void AddPostCommand(std::unique_ptr<DAVA::Command>&& command);
    const RECommandNotificationObject& GetMasterCommandInfo() const;

private:
    RECommandNotificationObject notifyObject;
    DAVA::Vector<std::unique_ptr<DAVA::Command>> preCommands;
    DAVA::Vector<std::unique_ptr<DAVA::Command>> postCommands;
};
