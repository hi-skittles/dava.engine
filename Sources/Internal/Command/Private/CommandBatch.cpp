#include "Command/CommandBatch.h"

#include "Debug/DVAssert.h"
#include "Reflection/ReflectionRegistrator.h"

#include <typeinfo>

namespace DAVA
{
namespace CommandBatchDetails
{
bool AreCommandTypesEqual(const Command& left, const Command& right)
{
    return typeid(left) == typeid(right);
}
}

CommandBatch::CommandBatch(const String& description, uint32 commandsCount)
    : Command(description)
{
    commandList.reserve(commandsCount);
}

void CommandBatch::Redo()
{
    for (auto it = commandList.begin(), end = commandList.end(); it != end; ++it)
    {
        (*it)->Redo();
    }
}

void CommandBatch::Undo()
{
    for (auto it = commandList.rbegin(), end = commandList.rend(); it != end; ++it)
    {
        (*it)->Undo();
    }
}

void CommandBatch::Add(std::unique_ptr<Command>&& command)
{
    DVASSERT(command);
    if (commandList.empty() == false)
    {
        DAVA::Command* lastCommand = commandList.back().get();
        if (CommandBatchDetails::AreCommandTypesEqual(*lastCommand, *command.get()))
        {
            if (lastCommand->MergeWith(command.get()))
            {
                return;
            }
        }
    }
    commandList.push_back(std::move(command));
}

void CommandBatch::AddAndRedo(std::unique_ptr<Command>&& command)
{
    DVASSERT(command);

    Command* actualCommand = command.get();
    Add(std::move(command));
    actualCommand->Redo();
}

const Command* CommandBatch::GetCommand(uint32 index) const
{
    DVASSERT(index < commandList.size());
    return commandList[index].get();
}

bool CommandBatch::IsClean() const
{
    return std::find_if(commandList.begin(), commandList.end(), [](const std::unique_ptr<Command>& command) {
               return command->IsClean() == false;
           }) == commandList.end();
}

bool IsCommandBatch(const Command* command)
{
    return dynamic_cast<const CommandBatch*>(command) != nullptr;
}

DAVA_VIRTUAL_REFLECTION_IMPL(CommandBatch)
{
    ReflectionRegistrator<CommandBatch>::Begin()
    .End();
}

} //namespace DAVA
