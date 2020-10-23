#pragma once

#include "Base/BaseTypes.h"
#include "Command/CommandStack.h"
#include "Commands2/Base/CommandNotify.h"

class RECommandStack : public DAVA::CommandStack, public CommandNotifyProvider
{
public:
    RECommandStack();
    ~RECommandStack() override;

    void Exec(std::unique_ptr<DAVA::Command>&& command) override;

    void Clear();
    void SetChanged();
    void RemoveCommands(DAVA::uint32 commandId);

    bool IsUncleanCommandExists(DAVA::uint32 commandId) const;

private:
    DAVA::CommandBatch* CreateCommmandBatch(const DAVA::String& name, DAVA::uint32 commandsCount) const override;

    void RemoveCommand(DAVA::uint32 index);

    void OnCommandExecuted(const DAVA::Command* cmd, bool redo);
    void ExecInternal(std::unique_ptr<DAVA::Command>&& command, bool isSingleCommand) override;
};
