#include "Commands2/Base/RECommandStack.h"
#include "Commands2/Base/RECommandBatch.h"
#include "Commands2/Base/CommandAction.h"

#include "Commands2/Base/RECommandNotificationObject.h"

RECommandStack::RECommandStack()
    : DAVA::CommandStack()
{
    commandExecuted.Connect(this, &RECommandStack::OnCommandExecuted);
}

RECommandStack::~RECommandStack() = default;

void RECommandStack::Clear()
{
    commands.clear();
    cleanIndex = EMPTY_INDEX;
    SetCurrentIndex(EMPTY_INDEX);
}

void RECommandStack::Exec(std::unique_ptr<DAVA::Command>&& command)
{
    RECommandNotificationObject notifyObject;
    if (DAVA::IsCommandBatch(command.get()))
    {
        notifyObject.batch = static_cast<const RECommandBatch*>(command.get());
    }
    else
    {
        notifyObject.command = static_cast<const RECommand*>(command.get());
    }
    notifyObject.redo = true;

    REDependentCommandsHolder holder(notifyObject);
    AccumulateDependentCommands(holder);

    bool hasDependentCommands = (holder.preCommands.empty() == false) || (holder.postCommands.empty() == false);
    bool singleCommandBatch = (commandBatch == nullptr) && hasDependentCommands;

    if (singleCommandBatch == true)
    {
        BeginBatch(command->GetDescription(), 1);
    }

    for (std::unique_ptr<DAVA::Command>& cmd : holder.preCommands)
    {
        Exec(std::move(cmd));
    }

    CommandStack::Exec(std::move(command));

    for (std::unique_ptr<DAVA::Command>& cmd : holder.postCommands)
    {
        Exec(std::move(cmd));
    }

    if (singleCommandBatch == true)
    {
        EndBatch();
    }
}

void RECommandStack::SetChanged()
{
    CommandStack::SetCleanState(false);
}

void RECommandStack::RemoveCommands(DAVA::uint32 commandId)
{
    for (DAVA::int32 index = static_cast<DAVA::int32>(commands.size() - 1); index >= 0; --index)
    {
        DAVA::Command* commandPtr = commands.at(index).get();
        if (DAVA::IsCommandBatch(commandPtr))
        {
            RECommandBatch* batch = static_cast<RECommandBatch*>(commandPtr);
            batch->RemoveCommands(commandId);
            if (batch->IsEmpty())
            {
                RemoveCommand(index);
            }
        }
        else
        {
            const RECommand* reCommand = static_cast<const RECommand*>(commandPtr);
            if (reCommand->GetID() == commandId)
            {
                RemoveCommand(index);
            }
        }
    }
}

bool RECommandStack::IsUncleanCommandExists(DAVA::uint32 commandId) const
{
    DAVA::uint32 size = static_cast<DAVA::uint32>(commands.size());
    for (DAVA::uint32 index = std::max(cleanIndex, 0); index < size; ++index)
    {
        const DAVA::Command* commandPtr = commands.at(index).get();
        if (IsCommandBatch(commandPtr) == false)
        {
            const RECommand* reCommandPtr = static_cast<const RECommand*>(commandPtr);
            if (reCommandPtr->MatchCommandID(commandId))
            {
                return true;
            }
        }
    }
    return false;
}

DAVA::CommandBatch* RECommandStack::CreateCommmandBatch(const DAVA::String& name, DAVA::uint32 commandsCount) const
{
    return new RECommandBatch(name, commandsCount);
}

void RECommandStack::RemoveCommand(DAVA::uint32 index)
{
    DVASSERT(index < commands.size());
    if (cleanIndex > static_cast<DAVA::int32>(index))
    {
        cleanIndex--;
    }
    commands.erase(commands.begin() + index);
    if (currentIndex > static_cast<DAVA::int32>(index))
    {
        SetCurrentIndex(currentIndex - 1);
    }
}

void RECommandStack::OnCommandExecuted(const DAVA::Command* command, bool redo)
{
    RECommandNotificationObject notification;
    if (DAVA::IsCommandBatch(command))
    {
        notification.batch = static_cast<const RECommandBatch*>(command);
    }
    else
    {
        notification.command = static_cast<const RECommand*>(command);
    }
    notification.redo = redo;
    EmitNotify(notification);
}

void RECommandStack::ExecInternal(std::unique_ptr<DAVA::Command>&& command, bool isSingleCommand)
{
    if (IsCommandAction(command.get()))
    {
        //get ownership of the given command;
        std::unique_ptr<DAVA::Command> commandAction(std::move(command));
        commandAction->Redo();
        OnCommandExecuted(commandAction.get(), true);

        if (!commandAction->IsClean())
        {
            SetChanged();
        }
    }
    else
    {
        CommandStack::ExecInternal(std::move(command), isSingleCommand);
    }
}
