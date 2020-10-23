#include "Command/CommandStack.h"
#include "Command/CommandBatch.h"
#include "Debug/DVAssert.h"

namespace DAVA
{
CommandStack::~CommandStack() = default;

void CommandStack::Exec(std::unique_ptr<Command>&& command)
{
    DVASSERT(command);
    if (command)
    {
        ExecInternal(std::move(command), true);
    }
}

void CommandStack::ExecInternal(std::unique_ptr<Command>&& command, bool isSingleCommand)
{
    if (commandBatch)
    {
        DVASSERT(isSingleCommand == true);
        commandBatch->AddAndRedo(std::move(command));
        UpdateCleanState();
    }
    else
    {
        DVASSERT(requestedBatchCount == 0);

        if (currentIndex != commands.size() - 1)
        {
            bool hasModifiedCommands = false;
            for (size_t i = static_cast<size_t>(currentIndex + 1); i < commands.size(); ++i)
            {
                if (commands[i]->IsClean() == false)
                {
                    hasModifiedCommands = true;
                    break;
                }
            }

            commands.erase(commands.begin() + (currentIndex + 1), commands.end());
            if (cleanIndex > currentIndex)
            {
                cleanIndex = EMPTY_INDEX;
                hasModifiedCommandsInRemoved = hasModifiedCommands;
            }
        }

        if (isSingleCommand)
        {
            command->Redo();
        }
        commands.push_back(std::move(command));
        SetCurrentIndex(currentIndex + 1);
        OnCurrentIndexChanged();
        //invoke it after SetCurrentIndex to discard logic problems, when client code trying to get IsClean, CanUndo or CanRedo after got commandExecuted
        commandExecuted.Emit(commands.back().get(), true);
    }
}

void CommandStack::BeginBatch(const String& name, uint32 commandsCount)
{
    if (requestedBatchCount == 0)
    {
        DVASSERT(!commandBatch);
        commandBatch.reset(CreateCommmandBatch(name, commandsCount));
    }
    DVASSERT(commandBatch);

    ++requestedBatchCount;
}

CommandBatch* CommandStack::CreateCommmandBatch(const String& name, uint32 commandsCount) const
{
    return new CommandBatch(name, commandsCount);
}

void CommandStack::EndBatch()
{
    DVASSERT(commandBatch, "CommandStack::EndMacro called without BeginMacro");
    DVASSERT(requestedBatchCount != 0, "CommandStack::EndMacro called without BeginMacro");

    --requestedBatchCount;
    if (requestedBatchCount == 0)
    {
        CommandBatch* commandBatchPtr = static_cast<CommandBatch*>(commandBatch.get());
        if (commandBatchPtr->IsEmpty())
        {
            commandBatch.reset();
        }
        else
        {
            //we need to release rootBatch before we will do something
            std::unique_ptr<CommandBatch> commandBatchCopy(std::move(commandBatch)); //do not remove this code!
            ExecInternal(std::move(commandBatchCopy), false);
        }
    }
}

bool CommandStack::IsClean() const
{
    return isClean;
}

void CommandStack::SetClean()
{
    cleanIndex = currentIndex;
    hasModifiedCommandsInRemoved = false;
    UpdateCleanState();
}

void CommandStack::Undo()
{
    if (CanUndo())
    {
        int32 commandIndexToExecute = currentIndex;
        SetCurrentIndex(currentIndex - 1);
        commands[commandIndexToExecute]->Undo();
        OnCurrentIndexChanged();
        //invoke it after SetCurrentIndex to discard logic problems, when client code trying to get IsClean, CanUndo or CanRedo after got commandExecuted
        commandExecuted.Emit(commands[commandIndexToExecute].get(), false);
    }
}

void CommandStack::Redo()
{
    if (CanRedo())
    {
        int32 commandIndexToExecute = currentIndex + 1;
        SetCurrentIndex(commandIndexToExecute);
        commands[commandIndexToExecute]->Redo();
        OnCurrentIndexChanged();
        //invoke it after SetCurrentIndex to discard logic problems, when client code trying to get IsClean, CanUndo or CanRedo after got commandExecuted
        commandExecuted.Emit(commands[commandIndexToExecute].get(), true);
    }
}

bool CommandStack::CanUndo() const
{
    return currentIndex > EMPTY_INDEX;
}

bool CommandStack::CanRedo() const
{
    return currentIndex < (static_cast<int32>(commands.size()) - 1);
}

const Command* CommandStack::GetUndoCommand() const
{
    if (CanUndo())
    {
        return commands[currentIndex].get();
    }
    return nullptr;
}

const Command* CommandStack::GetRedoCommand() const
{
    if (CanRedo())
    {
        return commands[currentIndex + 1].get();
    }
    return nullptr;
}

void CommandStack::UpdateCleanState()
{
    if (commandBatch != nullptr)
    {
        SetCleanState(commandBatch->IsClean() && isClean);
        return;
    }
    if (cleanIndex == currentIndex)
    {
        SetCleanState(true);
        return;
    }

    if (hasModifiedCommandsInRemoved == true)
    {
        SetCleanState(false);
        return;
    }
    int32 begin = std::min(cleanIndex, currentIndex);
    int32 end = std::max(cleanIndex, currentIndex);
    DVASSERT(end > begin);
    bool containsModifiedCommands = false;
    for (int32 index = begin; index != end && !containsModifiedCommands; ++index)
    {
        //we need to look only next commands after
        const std::unique_ptr<Command>& command = commands[index + 1];
        containsModifiedCommands |= (command->IsClean() == false);
    }
    SetCleanState(!containsModifiedCommands && isClean);
}

void CommandStack::SetCurrentIndex(int32 currentIndex_)
{
    if (currentIndex != currentIndex_)
    {
        currentIndex = currentIndex_;
    }
}

void CommandStack::OnCurrentIndexChanged()
{
    UpdateCleanState();
}

void CommandStack::SetCleanState(bool isClean_)
{
    isClean = isClean_;
}
}
