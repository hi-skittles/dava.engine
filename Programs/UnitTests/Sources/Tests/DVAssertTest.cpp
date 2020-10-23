#include "UnitTests/UnitTests.h"
#include "Debug/DVAssert.h"

using namespace DAVA;
using namespace DAVA::Assert;

static bool firstHandlerInvoked = false;
static FailBehaviour FirstHandler(const AssertInfo& assertInfo)
{
    firstHandlerInvoked = true;
    return FailBehaviour::Continue;
}

static bool secondHandlerInvoked = false;
static FailBehaviour SecondHandler(const AssertInfo& assertInfo)
{
    secondHandlerInvoked = true;
    return FailBehaviour::Continue;
}

static void ResetHandlersState()
{
    firstHandlerInvoked = false;
    secondHandlerInvoked = false;
}

static DAVA::String lastHandlerMessage;
static FailBehaviour AssertMessageSavingHandler(const AssertInfo& assertInfo)
{
    lastHandlerMessage = DAVA::String(assertInfo.message);
    return FailBehaviour::Continue;
}

// Guard to run test functions without previous handlers and to put them back when testing is over
class AssertsHandlersGuard final
{
public:
    AssertsHandlersGuard()
        : previousHandlers(GetAllHandlers())
    {
        RemoveAllHandlers();
    }

    ~AssertsHandlersGuard()
    {
        RemoveAllHandlers();
        for (const Handler& handler : previousHandlers)
        {
            AddHandler(handler);
        }
    }

private:
    const Vector<Handler> previousHandlers;
};

DAVA_TESTCLASS (DVAssertTestClass)
{
    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(DavaFramework)
    DECLARE_COVERED_FILES("DVAssert.cpp")
    END_FILES_COVERED_BY_TESTS()

    DAVA_TEST (AssertTestFunction)
    {
        AssertsHandlersGuard previousHandlersGuard;

        AddHandler(FirstHandler);
        AddHandler(SecondHandler);

        // If an assert doesn't fail, none should be called
        DVASSERT(true);
        TEST_VERIFY(!firstHandlerInvoked);
        TEST_VERIFY(!secondHandlerInvoked);

        // If it fails, both should be called
        DVASSERT(false);
#if defined(__DAVAENGINE_DEBUG__) || defined(__DAVAENGINE_ENABLE_ASSERTS__)
        TEST_VERIFY(firstHandlerInvoked);
        TEST_VERIFY(secondHandlerInvoked);
#else
        TEST_VERIFY(!firstHandlerInvoked);
        TEST_VERIFY(!secondHandlerInvoked);
#endif

        // Check that first removed handler doesn't get called
        RemoveHandler(FirstHandler);
        ResetHandlersState();
        DVASSERT(false);
#if defined(__DAVAENGINE_DEBUG__) || defined(__DAVAENGINE_ENABLE_ASSERTS__)
        TEST_VERIFY(!firstHandlerInvoked);
        TEST_VERIFY(secondHandlerInvoked);
#else
        TEST_VERIFY(!firstHandlerInvoked);
        TEST_VERIFY(!secondHandlerInvoked);
#endif

        // Reset
        ResetHandlersState();
    }

    DAVA_TEST (AlwaysAssertTestFunction)
    {
        AssertsHandlersGuard previousHandlersGuard;

        AddHandler(FirstHandler);
        AddHandler(SecondHandler);

        // If an assert doesn't fail, none should be called
        DVASSERT_ALWAYS(true);
        TEST_VERIFY(!firstHandlerInvoked);
        TEST_VERIFY(!secondHandlerInvoked);

        // If it fails, both should be called in both debug & release modes
        DVASSERT_ALWAYS(false);
        TEST_VERIFY(firstHandlerInvoked);
        TEST_VERIFY(secondHandlerInvoked);

        // Check that first removed handler doesn't get called
        RemoveHandler(FirstHandler);
        ResetHandlersState();
        DVASSERT_ALWAYS(false);
        TEST_VERIFY(!firstHandlerInvoked);
        TEST_VERIFY(secondHandlerInvoked);

        // Reset
        ResetHandlersState();
    }

    DAVA_TEST (AssertMessageTestFunction)
    {
        AssertsHandlersGuard previousHandlersGuard;

        AddHandler(AssertMessageSavingHandler);

        // Check that message is empty if none was specified
        DVASSERT_ALWAYS(false);
        TEST_VERIFY(lastHandlerMessage == "");

        // Check specified message
        DAVA::String message = "such assert, wow";
        DVASSERT_ALWAYS(false, message.c_str());
        TEST_VERIFY(lastHandlerMessage == message);
    }
};