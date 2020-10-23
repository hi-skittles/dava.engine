#include "Concurrency/Dispatcher.h"
#include "Concurrency/ManualResetEvent.h"
#include "Concurrency/Mutex.h"
#include "Concurrency/SyncBarrier.h"
#include "Concurrency/Thread.h"
#include "Engine/Engine.h"
#include "Functional/Function.h"
#include "UnitTests/UnitTests.h"

#include "Logger/Logger.h"
using namespace DAVA;

DAVA_TESTCLASS (DispatcherTest)
{
    using MyDispatcher = Dispatcher<Function<void()>>;

    DispatcherTest()
        : barrier(2)
    {
    }

    bool TestComplete(const String&)const override
    {
        if (testsComplete)
        {
            dispatcherThread->Join();
            dispatcherThread->Release();

            testThread->Join();
            testThread->Release();
        }
        return testsComplete;
    }

    DAVA_TEST (Test)
    {
        dispatcherThread = Thread::Create([this]() { DispatcherThread(); });
        dispatcherThread->Start();

        testThread = Thread::Create([this]() { TestThread(); });
        testThread->Start();
    }

    void TestThread()
    {
        barrier.Wait();

        // Post tasks to increment counter by 1
        for (int i = 0; i < 10; ++i)
        {
            dispatcher->PostEvent([this]() { counter += 1; });
        }

        // Do blocking call to ensure previous posts are processed
        dispatcher->SendEvent([]() {});
        TEST_VERIFY(counter == 10);

        // Wait each event procession
        int testWithMe = 0;
        for (int i = 0; i < 10; ++i)
        {
            testWithMe += i;
            dispatcher->SendEvent([this, i]() { counter2 += i; });
            TEST_VERIFY(counter2 == testWithMe);
        }

        dispatcher->PostEvent([this]() {
            // Post events from dispatcher's thread
            for (int i = 0; i < 10; ++i)
            {
                dispatcher->PostEvent([this]() { counter3 += 1; });
            }
            dispatcher->PostEvent([this]() { barrier.Wait(); });

            // Send out-of-band event which should be executed first as send is performed in dispatcher's thread
            int testMe = 0;
            dispatcher->SendEvent([&testMe]() { testMe = 42; }, MyDispatcher::eSendPolicy::IMMEDIATE_EXECUTION);
            TEST_VERIFY(testMe == 42);
            TEST_VERIFY(counter3 == 0);
        });

        barrier.Wait();
        // This thread and dispatcher's thread should come here simultaneously
        // Dispatcher guarantees sequential order of event processing so counter3 should be 10
        TEST_VERIFY(counter3 == 10);

        // Tell dispatcher's thread to exit
        dispatcher->PostEvent([this]() { byeDispatcherThread = true; });
        RunOnMainThreadAsync([this]() { testsComplete = true; });
    }

    void DispatcherThread()
    {
        dispatcher = new MyDispatcher([](const Function<void()>& fn) { fn(); });
        dispatcher->LinkToCurrentThread();
        barrier.Wait();

        while (!byeDispatcherThread)
        {
            dispatcher->ProcessEvents();
            Thread::Sleep(50);
        }

        delete dispatcher;
        dispatcher = nullptr;
    }

    SyncBarrier barrier;
    MyDispatcher* dispatcher = nullptr;
    Thread* testThread = nullptr;
    Thread* dispatcherThread = nullptr;

    bool testsComplete = false;
    bool byeDispatcherThread = false;

    int counter = 0;
    int counter2 = 0;
    int counter3 = 0;
};
