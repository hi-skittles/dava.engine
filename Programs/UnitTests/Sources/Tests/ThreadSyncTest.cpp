#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"
#include "Concurrency/AutoResetEvent.h"
#include "Concurrency/ManualResetEvent.h"
#include <random>

using namespace DAVA;

DAVA_TESTCLASS (ThreadSyncTest)
{
    Thread* someThread = nullptr;

    Mutex cvMutex;
    ConditionVariable cv;
    int someValue;

    volatile int autoResetValue;
    volatile int manualResetValue;
    std::atomic<int> autoResetWakeCounter;
    Mutex autoResetMutex;

    static const int autoResetLoopCount = 10000;
    static const int autoResetThreadCount = 4;

    AutoResetEvent are;
    ManualResetEvent mre;

    Vector<Thread*> currentThreads;

    DAVA_TEST (ThreadCurrentTest)
    {
        // Thread subsystem maintains list of created Thread instances, this list is used
        // in Thread::Current() method to retrieve Thread instance for current thread.
        // Note that list is searched by thread id but system can reuse that id for newly created
        // thread and Thread::Current() can return Thread instance for different thread.
        // System considers thread is finished and can reuse its id after joining.
        const size_t N = 50;
        for (size_t i = 0; i < N; ++i)
        {
            Thread* t = Thread::Create([this, i]() {
                Thread* cur = Thread::Current();
                Thread* test = currentThreads[i];

                TEST_VERIFY(cur == test);
            });

            currentThreads.push_back(t);
            t->Start();
            t->Join();
        }

        for (Thread* t : currentThreads)
            t->Release();
        currentThreads.clear();
    }

    DAVA_TEST (ThreadJoinableTest)
    {
        RefPtr<Thread> p(Thread::Create([]() {
            Thread::Sleep(2000);
        }));

        TEST_VERIFY(p->IsJoinable() == false);
        p->Start();
        TEST_VERIFY(p->IsJoinable() == true);
        p->Join();
        TEST_VERIFY(p->IsJoinable() == false);
    }

    DAVA_TEST (ThreadSyncTestFunction)
    {
        cvMutex.Lock();
        someThread = Thread::Create(MakeFunction(this, &ThreadSyncTest::SomeThreadFunc));
        someValue = -1;
        someThread->Start();
        cv.Wait(cvMutex);
        cvMutex.Unlock();

        TEST_VERIFY(someValue == 0);
        someThread->Join();
        TEST_VERIFY(0 == someThread->Release());
        someThread = nullptr;
    }

    DAVA_TEST (TestThread)
    {
        TEST_VERIFY(true == Thread::IsMainThread());

        Thread* infiniteThread = Thread::Create(MakeFunction(this, &ThreadSyncTest::InfiniteThreadFunction));

        TEST_VERIFY(Thread::STATE_CREATED == infiniteThread->GetState());
        infiniteThread->SetName("Infinite test thread");
        infiniteThread->Start();

        uint32 timeout = 3000;
        while (timeout-- > 0 && Thread::STATE_RUNNING != infiniteThread->GetState())
        {
            Thread::Sleep(1);
        }
        TEST_VERIFY(timeout > 0);

        infiniteThread->Cancel();
        infiniteThread->Join();
        TEST_VERIFY(Thread::STATE_ENDED == infiniteThread->GetState());

        Thread* shortThread = Thread::Create(MakeFunction(this, &ThreadSyncTest::ShortThreadFunction));
        shortThread->Start();
        shortThread->Join();
        TEST_VERIFY(Thread::STATE_ENDED == shortThread->GetState());

        TEST_VERIFY(0 == shortThread->Release());
        shortThread = NULL;
        TEST_VERIFY(0 == infiniteThread->Release());
        infiniteThread = NULL;
        /*
        //  Disable because it affects all threads including JobManager threads and worker threads,
        //  so this test is intrusive
        infiniteThread = Thread::Create(Message(this, &ThreadSyncTest::InfiniteThreadFunction));
        shortThread = Thread::Create(Message(this, &ThreadSyncTest::ShortThreadFunction));

        infiniteThread->Start();
        shortThread->Start();

        Thread::Sleep(50);
        timeout = 3000;
        while (timeout-- > 0
        && Thread::STATE_RUNNING != infiniteThread->GetState()
        && Thread::STATE_RUNNING != shortThread->GetState())
        {
        Thread::Sleep(1);
        }

        Thread::Id itid = infiniteThread->GetId();
        Thread::Id stid = shortThread->GetId();
        TEST_VERIFY(itid != stid);

        infiniteThread->Kill();
        TEST_VERIFY(0 == shortThread->Release());

        shortThread->Kill();
        TEST_VERIFY(0 == shortThread->Release());

        Thread::KillAll();

        shortThread->Join();
        TEST_VERIFY(Thread::STATE_KILLED == shortThread->GetState());

        infiniteThread->Join();
        TEST_VERIFY(Thread::STATE_KILLED == infiniteThread->GetState());

        TEST_VERIFY(0 != shortThread);
        TEST_VERIFY(0 == shortThread->Release());
        shortThread = NULL;

        TEST_VERIFY(0 == infiniteThread->Release());
        infiniteThread = NULL;

        Logger::Debug("[ThreadSyncTest] Done.");
        */
    }

    DAVA_TEST (TestAutoResetEvent)
    {
        Thread* threads[autoResetThreadCount];

        autoResetValue = 0;
        for (int i = 0; i < autoResetThreadCount; ++i)
        {
            threads[i] = Thread::Create(MakeFunction(this, &ThreadSyncTest::AutoResetEventThreadFunc));
            threads[i]->Start();
        }

        // test that only one thread will wake if we signaling once
        {
            autoResetWakeCounter.store(0);
            are.Signal();

            // unlock threads and test
            Thread::Sleep(500);
            TEST_VERIFY(autoResetWakeCounter.load() == 1);
        }

        // test that only one thread will wake if we signaling multiple times
        {
            // make sure all thread are not waiting on autoResetMutex
            {
                autoResetMutex.Lock();
                for (int i = 0; i < autoResetThreadCount; ++i)
                {
                    are.Signal();
                    Thread::Sleep(500);
                }
            }

            // signal multiple times
            autoResetWakeCounter.store(0);
            for (int i = 0; i < autoResetThreadCount; ++i) are.Signal();

            autoResetMutex.Unlock();
            Thread::Sleep(500);
            TEST_VERIFY(autoResetWakeCounter.load() == 1);
        }

        // continue signaling until all thread finish their work
        while (autoResetValue < autoResetThreadCount)
        {
            are.Signal();
        }

        for (int i = 0; i < autoResetThreadCount; ++i)
        {
            threads[i]->Join();
            threads[i]->Release();
        }
    }

    void AutoResetEventThreadFunc()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0, 1);
        double res = 0;

        for (int i = 0; i < autoResetLoopCount; ++i)
        {
            res += dis(gen);

            are.Wait();
            autoResetWakeCounter++;

            autoResetMutex.Lock();
            autoResetMutex.Unlock();
        }

        autoResetValue++;
    }

    DAVA_TEST (TestManualResetEvent)
    {
        Thread* threads[autoResetThreadCount];

        manualResetValue = 0;
        for (int i = 0; i < autoResetThreadCount; ++i)
        {
            threads[i] = Thread::Create(MakeFunction(this, &ThreadSyncTest::ManualResetEventThreadFunc));
            threads[i]->Start();
        }

        // check that wait don't hang if manualResetEvent is signaled
        autoResetMutex.Lock();
        Thread::Sleep(500);
        int check = autoResetWakeCounter;
        mre.Wait();
        mre.Wait();
        mre.Wait();
        Thread::Sleep(500);
        TEST_VERIFY(check == autoResetWakeCounter);
        autoResetMutex.Unlock();

        while (manualResetValue < autoResetThreadCount)
        {
            mre.Wait();
        }

        for (int i = 0; i < autoResetThreadCount; ++i)
        {
            threads[i]->Join();
            threads[i]->Release();
        }
    }

    void ManualResetEventThreadFunc()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0, 1);
        double res = 0;

        for (int i = 0; i < autoResetLoopCount; ++i)
        {
            mre.Reset();
            res += dis(gen);
            mre.Signal();
            autoResetWakeCounter++;

            autoResetMutex.Lock();
            autoResetMutex.Unlock();
        }

        manualResetValue++;
        Logger::Info("%f", res);
    }

    void SomeThreadFunc()
    {
        someValue = 0;
        cvMutex.Lock();
        cv.NotifyOne();
        cvMutex.Unlock();
    }

    void InfiniteThreadFunction()
    {
        Thread* thread = Thread::Current();
        while (thread && !thread->IsCancelling())
        {
            Thread::Sleep(200);
        }
    }

    void ShortThreadFunction()
    {
        uint32 i = 200;
        Thread* thread = Thread::Current();
        while (thread && i-- > 0)
        {
            Thread::Sleep(1);
            if (thread->IsCancelling())
                break;
        }
    }

    static void StackHurtFunc()
    {
        const int theGreatestNumber = 42;
        volatile char data[1 * 1024 * 1024]; //1 MB
        volatile int sum = 0;

        for (auto& x : data)
        {
            sum += theGreatestNumber;
            x = sum;
        }
    }

    //if stack size is not set, app will crash
    DAVA_TEST (StackHurtTest)
    {
        auto stackHurtThread = RefPtr<Thread>(Thread::Create(&StackHurtFunc));
        stackHurtThread->SetStackSize(2 * 1024 * 1024); //2 MB
        stackHurtThread->Start();
        stackHurtThread->Join();
    }
};
