#include "Classes/Commands2/Base/RECommandNotificationObject.h"
#include "Classes/Commands2/Base/RECommandIDHandler.h"
#include "Classes/Commands2/RECommandIDs.h"

namespace RECommandNotificationObjectDetail
{
const RECommandIDHandler* GetIDHandler(const RECommandNotificationObject* object)
{
    const RECommandIDHandler* batchHandler = object->batch;
    const RECommandIDHandler* commandHandler = object->command;

    return (batchHandler != nullptr) ? batchHandler : commandHandler;
}
}

bool RECommandNotificationObject::IsEmpty() const
{
    return (batch == nullptr) && (command == nullptr);
}

void RECommandNotificationObject::ExecuteForAllCommands(const DAVA::Function<void(const RECommand*, bool)>& fn) const
{
    if (batch != nullptr)
    {
        for (DAVA::uint32 i = 0, count = batch->Size(); i < count; ++i)
        {
            fn(batch->GetCommand(i), redo);
        }
    }
    else
    {
        fn(command, redo);
    }
}

bool RECommandNotificationObject::MatchCommandID(DAVA::uint32 commandID) const
{
    if (IsEmpty())
        return false;

    const RECommandIDHandler* idHandler = RECommandNotificationObjectDetail::GetIDHandler(this);
    return idHandler->MatchCommandID(commandID);
}

bool RECommandNotificationObject::MatchCommandIDs(const DAVA::Vector<DAVA::uint32>& commandIDVector) const
{
    if (IsEmpty())
        return false;

    const RECommandIDHandler* idHandler = RECommandNotificationObjectDetail::GetIDHandler(this);
    return idHandler->MatchCommandIDs(commandIDVector);
}

void RECommandNotificationObject::ForEach(const DAVA::Function<void(const RECommand*)>& callback, DAVA::uint32 commandId) const
{
    if (command != nullptr && command->GetID() == commandId)
    {
        callback(command);
    }

    auto batchUnpack = [&](const RECommandBatch* batch)
    {
        if (batch != nullptr)
        {
            for (DAVA::uint32 i = 0; i < batch->Size(); ++i)
            {
                const RECommand* command = batch->GetCommand(i);
                if (command->GetID() == commandId)
                {
                    callback(command);
                }
            }
        }
    };

    batchUnpack(batch);
}

REDependentCommandsHolder::REDependentCommandsHolder(const RECommandNotificationObject& notifyObject_)
    : notifyObject(notifyObject_)
{
}

void REDependentCommandsHolder::AddPreCommand(std::unique_ptr<DAVA::Command>&& command)
{
    preCommands.push_back(std::move(command));
}

void REDependentCommandsHolder::AddPostCommand(std::unique_ptr<DAVA::Command>&& command)
{
    postCommands.push_back(std::move(command));
}

const RECommandNotificationObject& REDependentCommandsHolder::GetMasterCommandInfo() const
{
    return notifyObject;
}
