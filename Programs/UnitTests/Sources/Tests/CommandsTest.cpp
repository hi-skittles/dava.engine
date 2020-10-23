#include "UnitTests/UnitTests.h"

#include <Command/Command.h>
#include <Command/CommandBatch.h>
#include <Command/CommandStack.h>
#include <Reflection/ReflectedTypeDB.h>
#include <Debug/DVAssert.h>

class TestCommand : public DAVA::Command
{
public:
    TestCommand(const DAVA::String& description = "")
        : DAVA::Command(description)
    {
    }

    void Redo() override
    {
        ++redoCounter;
    }

    void Undo() override
    {
        ++undoCounter;
    }

    DAVA::uint32 redoCounter = 0;
    DAVA::uint32 undoCounter = 0;
};

class TestCommandClean : public TestCommand
{
public:
    TestCommandClean(const DAVA::String& description = "")
        : TestCommand(description)
    {
    }
    bool IsClean() const override
    {
        return true;
    }
};

class TestCommandMerge : public DAVA::Command
{
public:
    TestCommandMerge()
        : DAVA::Command()
    {
    }

    void Redo() override
    {
        ++redoCounter;
    }

    void Undo() override
    {
        ++undoCounter;
    }

    bool MergeWith(const DAVA::Command* another) override
    {
        DVASSERT(dynamic_cast<const TestCommandMerge*>(another) != nullptr);
        return true;
    }

    static DAVA::uint32 redoCounter;
    static DAVA::uint32 undoCounter;
};

DAVA::uint32 TestCommandMerge::redoCounter = 0;
DAVA::uint32 TestCommandMerge::undoCounter = 0;

DAVA_TESTCLASS (CommandsTest)
{
    BEGIN_FILES_COVERED_BY_TESTS()
    DECLARE_COVERED_FILES("Command.cpp")
    DECLARE_COVERED_FILES("CommandBatch.cpp")
    DECLARE_COVERED_FILES("CommandStack.cpp")
    END_FILES_COVERED_BY_TESTS()

    DAVA_TEST (SimpleTest)
    {
        DAVA::ReflectedTypeDB::Get<DAVA::Command>();
        std::unique_ptr<DAVA::Command> command(new TestCommand("TestCommand"));
        TEST_VERIFY(command->GetDescription() == "TestCommand");
        TEST_VERIFY(command->IsClean() == false);

        command->Redo();
        command->Undo();
        command->Redo();

        TestCommand* testCommand = static_cast<TestCommand*>(command.get());
        TEST_VERIFY(testCommand->redoCounter == 2);
        TEST_VERIFY(testCommand->undoCounter == 1);

        std::unique_ptr<DAVA::Command> commandClean(new TestCommandClean("TestCommandClean"));
        TEST_VERIFY(commandClean->GetDescription() == "TestCommandClean");
        TEST_VERIFY(commandClean->IsClean() == true);
    }

    DAVA_TEST (CommandStackTest)
    {
        DAVA::CommandStack stack;
        TEST_VERIFY(stack.IsClean() == true);
        TEST_VERIFY(stack.CanUndo() == false);
        TEST_VERIFY(stack.CanRedo() == false);

        std::unique_ptr<DAVA::Command> command(new TestCommand("TestCommand"));
        TestCommand* testCommand = static_cast<TestCommand*>(command.get());
        stack.Exec(std::move(command));
        TEST_VERIFY(testCommand->redoCounter == 1);
        TEST_VERIFY(testCommand->undoCounter == 0);
        TEST_VERIFY(stack.IsClean() == false);
        TEST_VERIFY(stack.CanUndo() == true);
        TEST_VERIFY(stack.CanRedo() == false);

        stack.Undo();
        TEST_VERIFY(testCommand->redoCounter == 1);
        TEST_VERIFY(testCommand->undoCounter == 1);
        TEST_VERIFY(stack.IsClean() == true);
        TEST_VERIFY(stack.CanUndo() == false);
        TEST_VERIFY(stack.CanRedo() == true);

        stack.Redo();
        TEST_VERIFY(testCommand->redoCounter == 2);
        TEST_VERIFY(testCommand->undoCounter == 1);
        TEST_VERIFY(stack.IsClean() == false);
        TEST_VERIFY(stack.CanUndo() == true);
        TEST_VERIFY(stack.CanRedo() == false);

        stack.Exec(std::unique_ptr<DAVA::Command>(new TestCommand("TestCommand2")));
        TEST_VERIFY(testCommand->redoCounter == 2);
        TEST_VERIFY(testCommand->undoCounter == 1);
        TEST_VERIFY(stack.IsClean() == false);
        TEST_VERIFY(stack.CanUndo() == true);
        TEST_VERIFY(stack.CanRedo() == false);

        stack.Undo();
        TEST_VERIFY(testCommand->redoCounter == 2);
        TEST_VERIFY(testCommand->undoCounter == 1);
        TEST_VERIFY(stack.IsClean() == false);
        TEST_VERIFY(stack.CanUndo() == true);
        TEST_VERIFY(stack.CanRedo() == true);

        stack.Undo();
        TEST_VERIFY(testCommand->redoCounter == 2);
        TEST_VERIFY(testCommand->undoCounter == 2);
        TEST_VERIFY(stack.IsClean() == true);
        TEST_VERIFY(stack.CanUndo() == false);
        TEST_VERIFY(stack.CanRedo() == true);

        stack.Redo();
        TEST_VERIFY(testCommand->redoCounter == 3);
        TEST_VERIFY(testCommand->undoCounter == 2);
        TEST_VERIFY(stack.IsClean() == false);
        TEST_VERIFY(stack.CanUndo() == true);
        TEST_VERIFY(stack.CanRedo() == true);

        stack.Redo();
        TEST_VERIFY(testCommand->redoCounter == 3);
        TEST_VERIFY(testCommand->undoCounter == 2);
        TEST_VERIFY(stack.IsClean() == false);
        TEST_VERIFY(stack.CanUndo() == true);
        TEST_VERIFY(stack.CanRedo() == false);

        stack.Undo();
        stack.Exec(std::unique_ptr<DAVA::Command>(new TestCommand("TestCommand3")));
        TEST_VERIFY(testCommand->redoCounter == 3);
        TEST_VERIFY(testCommand->undoCounter == 2);
        TEST_VERIFY(stack.IsClean() == false);
        TEST_VERIFY(stack.CanUndo() == true);
        TEST_VERIFY(stack.CanRedo() == false);
    }

    DAVA_TEST (CommandStackCleanTest)
    {
        DAVA::CommandStack stack;
        stack.Exec(std::unique_ptr<DAVA::Command>(new TestCommandClean()));
        TEST_VERIFY(stack.IsClean() == true);
        TEST_VERIFY(stack.CanUndo() == true);
        TEST_VERIFY(stack.CanRedo() == false);

        stack.Exec(std::unique_ptr<DAVA::Command>(new TestCommand()));
        TEST_VERIFY(stack.IsClean() == false);
        TEST_VERIFY(stack.CanUndo() == true);
        TEST_VERIFY(stack.CanRedo() == false);

        stack.SetClean();
        TEST_VERIFY(stack.IsClean() == true);
        TEST_VERIFY(stack.CanUndo() == true);
        TEST_VERIFY(stack.CanRedo() == false);

        stack.Undo();
        TEST_VERIFY(stack.IsClean() == false);
        TEST_VERIFY(stack.CanUndo() == true);
        TEST_VERIFY(stack.CanRedo() == true);
    }

    DAVA_TEST (CommandBatchSimpleTest)
    {
        std::unique_ptr<DAVA::CommandBatch> testBatch(new DAVA::CommandBatch("TestBatch", 2));
        TEST_VERIFY(DAVA::IsCommandBatch(testBatch.get()) == true);
        TEST_VERIFY(testBatch->Size() == 0);
        TEST_VERIFY(testBatch->IsEmpty());

        std::unique_ptr<DAVA::Command> batchTestCommand(new TestCommand("BatchTestCommand"));
        TestCommand* testCommandPtr = static_cast<TestCommand*>(batchTestCommand.get());
        TEST_VERIFY(DAVA::IsCommandBatch(testCommandPtr) == false);

        std::unique_ptr<DAVA::Command> batchTestCommand2(new TestCommand("BatchTestCommand2"));
        TestCommand* testCommand2Ptr = static_cast<TestCommand*>(batchTestCommand2.get());

        testBatch->AddAndRedo(std::move(batchTestCommand));
        testBatch->Add(std::move(batchTestCommand2));

        TEST_VERIFY(testCommandPtr->redoCounter == 1);
        TEST_VERIFY(testCommandPtr->undoCounter == 0);
        TEST_VERIFY(testCommand2Ptr->redoCounter == 0);
        TEST_VERIFY(testCommand2Ptr->undoCounter == 0);

        testBatch->Redo();
        TEST_VERIFY(testCommandPtr->redoCounter == 2);
        TEST_VERIFY(testCommandPtr->undoCounter == 0);
        TEST_VERIFY(testCommand2Ptr->redoCounter == 1);
        TEST_VERIFY(testCommand2Ptr->undoCounter == 0);

        testBatch->Undo();
        TEST_VERIFY(testCommandPtr->redoCounter == 2);
        TEST_VERIFY(testCommandPtr->undoCounter == 1);
        TEST_VERIFY(testCommand2Ptr->redoCounter == 1);
        TEST_VERIFY(testCommand2Ptr->undoCounter == 1);
    }

    DAVA_TEST (CommandBatchTest)
    {
        std::unique_ptr<DAVA::Command> command(new TestCommand("TestCommand"));
        TestCommand* testCommand = static_cast<TestCommand*>(command.get());

        DAVA::CommandStack stack;
        stack.BeginBatch("TestBatch", 1);

        stack.Exec(std::move(command));
        TEST_VERIFY(testCommand->redoCounter == 1);
        TEST_VERIFY(testCommand->undoCounter == 0);

        stack.EndBatch();
        TEST_VERIFY(testCommand->redoCounter == 1);
        TEST_VERIFY(testCommand->undoCounter == 0);
        TEST_VERIFY(stack.IsClean() == false);
        TEST_VERIFY(stack.CanUndo() == true);
        TEST_VERIFY(stack.CanRedo() == false);

        stack.Undo();
        TEST_VERIFY(testCommand->redoCounter == 1);
        TEST_VERIFY(testCommand->undoCounter == 1);
        TEST_VERIFY(stack.IsClean() == true);
        TEST_VERIFY(stack.CanUndo() == false);
        TEST_VERIFY(stack.CanRedo() == true);

        stack.Redo();
        TEST_VERIFY(testCommand->redoCounter == 2);
        TEST_VERIFY(testCommand->undoCounter == 1);
        TEST_VERIFY(stack.IsClean() == false);
        TEST_VERIFY(stack.CanUndo() == true);
        TEST_VERIFY(stack.CanRedo() == false);
    }

    DAVA_TEST (CommandBatchInnerTest)
    {
        std::unique_ptr<DAVA::Command> command(new TestCommand("TestCommand"));
        TestCommand* testCommand = static_cast<TestCommand*>(command.get());
        TestCommand* testCommandInner = nullptr;

        DAVA::CommandStack stack;
        stack.BeginBatch("RootBatch", 1);
        stack.Exec(std::move(command));

        {
            stack.BeginBatch("InnerBatch", 2);

            std::unique_ptr<DAVA::Command> innerCommand(new TestCommand("TestCommand_inner"));
            testCommandInner = static_cast<TestCommand*>(innerCommand.get());

            stack.Exec(std::move(innerCommand));
            TEST_VERIFY(testCommandInner->redoCounter == 1);
            TEST_VERIFY(testCommandInner->undoCounter == 0);

            stack.Exec(std::unique_ptr<DAVA::Command>(new TestCommandClean("TestCommand_inner_clean")));
            TEST_VERIFY(testCommandInner->redoCounter == 1);
            TEST_VERIFY(testCommandInner->undoCounter == 0);

            stack.EndBatch();
            TEST_VERIFY(testCommandInner->redoCounter == 1);
            TEST_VERIFY(testCommandInner->undoCounter == 0);
        }

        stack.EndBatch();
        TEST_VERIFY(testCommand->redoCounter == 1);
        TEST_VERIFY(testCommand->undoCounter == 0);
        TEST_VERIFY(testCommandInner->redoCounter == 1);
        TEST_VERIFY(testCommandInner->undoCounter == 0);
        TEST_VERIFY(stack.IsClean() == false);
        TEST_VERIFY(stack.CanUndo() == true);
        TEST_VERIFY(stack.CanRedo() == false);

        stack.Undo();
        TEST_VERIFY(testCommand->redoCounter == 1);
        TEST_VERIFY(testCommand->undoCounter == 1);
        TEST_VERIFY(testCommandInner->redoCounter == 1);
        TEST_VERIFY(testCommandInner->undoCounter == 1);
        TEST_VERIFY(stack.IsClean() == true);
        TEST_VERIFY(stack.CanUndo() == false);
        TEST_VERIFY(stack.CanRedo() == true);

        stack.Redo();
        TEST_VERIFY(testCommand->redoCounter == 2);
        TEST_VERIFY(testCommand->undoCounter == 1);
        TEST_VERIFY(testCommandInner->redoCounter == 2);
        TEST_VERIFY(testCommandInner->undoCounter == 1);
        TEST_VERIFY(stack.IsClean() == false);
        TEST_VERIFY(stack.CanUndo() == true);
        TEST_VERIFY(stack.CanRedo() == false);
    }

    DAVA_TEST (CommandBatchMergeTest)
    {
        DAVA::CommandStack stack;
        stack.BeginBatch("RootBatch", 1);
        stack.Exec(std::make_unique<TestCommandMerge>());
        stack.Exec(std::make_unique<TestCommandMerge>());
        stack.EndBatch();
        TEST_VERIFY(TestCommandMerge::redoCounter == 2);
        TEST_VERIFY(stack.IsClean() == false);

        stack.Undo();
        TEST_VERIFY(TestCommandMerge::undoCounter == 1);
        TEST_VERIFY(stack.IsClean());

        stack.Redo();
        TEST_VERIFY(TestCommandMerge::redoCounter == 3);
        TEST_VERIFY(stack.IsClean() == false);
    }
};
