#pragma once

#include "Command/Command.h"
#include "Command/CommandBatch.h"
#include "Functional/Signal.h"

namespace DAVA
{
class CommandStack
{
public:
    virtual ~CommandStack();

    virtual void Exec(std::unique_ptr<Command>&& command);

    void BeginBatch(const String& name, uint32 commandsCount = 1);
    void EndBatch();

    virtual bool IsClean() const;
    virtual void SetClean();

    void Undo();
    void Redo();

    bool CanUndo() const;
    bool CanRedo() const;

    const Command* GetUndoCommand() const;
    const Command* GetRedoCommand() const;
    Signal<const Command*, bool /*redo*/> commandExecuted;

protected:
    virtual CommandBatch* CreateCommmandBatch(const String& name, uint32 commandsCount) const;

    virtual void ExecInternal(std::unique_ptr<Command>&& command, bool isSingleCommand);

    void UpdateCleanState();
    void SetCurrentIndex(int32 currentIndex);
    void OnCurrentIndexChanged();

    static const int32 EMPTY_INDEX = -1;

    int32 cleanIndex = EMPTY_INDEX;
    int32 currentIndex = EMPTY_INDEX;

    Vector<std::unique_ptr<Command>> commands;

    std::unique_ptr<CommandBatch> commandBatch;
    uint32 requestedBatchCount = 0;

    //members to remember stack state and do not emit extra signals
    void SetCleanState(bool isClean);

    bool isClean = true;
    bool hasModifiedCommandsInRemoved = false;
};
}
