#pragma once

#include "Command/Command.h"

namespace DAVA
{
class CommandBatch : public Command
{
public:
    /**
    \brief Creates an empty command batch with required name.
    \param[in] text command batch text description to be displayed in widgets / network packets / log texts.
    \param[in] commandsCoubnt commands count to reserve memory to optimize memory allocation count.
    */
    CommandBatch(const String& description = "", uint32 commandsCount = 1);

    /**
    \brief Calls Redo to the all commands in batch.
    */
    void Redo() override;

    /**
    \brief Calls Undo to the all commands in batch in a reverse order.
    */
    void Undo() override;

    /**
     \brief Moves command to the batch.
     */
    void Add(std::unique_ptr<Command>&& command);

    /**
    \brief Moves command to the batch and calls Redo to the moved command.
    */
    void AddAndRedo(std::unique_ptr<Command>&& command);

    /**
     \brief Returns whether the batch is empty (i.e. whether its size is 0)
     \returns true if batch size is 0, false otherwise.
     */
    bool IsEmpty() const;

    /**
     \brief Returns the number of commands in the batch.
     \returns The number of commands in the batch.
     */
    uint32 Size() const;

    /**
    \returns Returns command at index \c index.
    */
    const Command* GetCommand(uint32 index) const;

    /**
     \brief Works the same as Command::IsClean
     \returns true if empty or contain only clean commands
     */
    bool IsClean() const override;

protected:
    using CommandsContainer = Vector<std::unique_ptr<Command>>;
    CommandsContainer commandList;

    DAVA_VIRTUAL_REFLECTION(CommandBatch, Command);
};

inline bool CommandBatch::IsEmpty() const
{
    return commandList.empty();
}

inline uint32 CommandBatch::Size() const
{
    return static_cast<uint32>(commandList.size());
}

bool IsCommandBatch(const Command* command);
} //namespace DAVA
