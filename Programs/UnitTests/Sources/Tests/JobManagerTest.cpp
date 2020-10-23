#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

using namespace DAVA;

#define JOBS_COUNT 500

struct JobManagerTestData
{
    JobManagerTestData()
        : mainThreadVar(0)
        , testThreadVar(0)
        , ownedMainJobsVar(-1)
    {
    }
    uint32 mainThreadVar;
    uint32 testThreadVar;

    int32 ownedMainJobsVar;
};

class TestJobOwner : public BaseObject
{
public:
    TestJobOwner(int32* _outData)
        : resultData(_outData)
        , anyData(0)
    {
    }
    virtual ~TestJobOwner()
    {
        (*resultData) = anyData;
    };

    void AnyFunction()
    {
        anyData++;
    };

protected:
    int32* resultData;
    Atomic<int32> anyData;
};

static void testCalc(uint32* var)
{
    (*var)++;

    {
        uint32 t = 0;
        uint32 v = *var;

        for (uint32 i = 1; i <= v; ++i)
        {
            t += (i * v) + (t * i);
        }

        *var += ((t & 0x3) - (t & 0x1));
    }
}

DAVA_TESTCLASS (JobManagerTest)
{
    DEDUCE_COVERED_FILES_FROM_TESTCLASS()

    DAVA_TEST (TestMainJobs)
    {
        JobManagerTestData testData;

        Thread* thread = Thread::Create([this, &testData]() { this->ThreadFunc(&testData); });
        thread->Start();

        while (thread->GetState() != Thread::STATE_ENDED)
        {
            GetEngineContext()->jobManager->Update();
        }

        thread->Join();

        TEST_VERIFY((testData.mainThreadVar == testData.testThreadVar));
        TEST_VERIFY((testData.ownedMainJobsVar == JOBS_COUNT));
    }

    DAVA_TEST (TestWorkerJobs)
    {
        // TODO:
        // ...
    }

    void ThreadFunc(JobManagerTestData * data)
    {
        for (uint32 i = 0; i < JOBS_COUNT; i++)
        {
            uint32 count = 50;
            uint32 n = Random::Instance()->Rand(count);
            uint32 jobId = 0;

            for (uint32 j = 0; j < count; ++j)
            {
                // calculate in main thread
                Function<void()> fn = std::bind(&testCalc, &data->mainThreadVar);
                uint32 id = GetEngineContext()->jobManager->CreateMainJob(fn);

                if (j == n)
                {
                    jobId = id;
                }
            }

            GetEngineContext()->jobManager->WaitMainJobID(jobId);

            for (uint32 j = 0; j < count; ++j)
            {
                // calculate in this thread
                testCalc(&data->testThreadVar);
            }
        }

        TestJobOwner* jobOwner = new TestJobOwner(&data->ownedMainJobsVar);
        for (uint32 i = 0; i < JOBS_COUNT; ++i)
        {
            GetEngineContext()->jobManager->CreateMainJob(MakeFunction(MakeSharedObject(jobOwner), &TestJobOwner::AnyFunction));
        }
        jobOwner->Release();
        GetEngineContext()->jobManager->WaitMainJobs();
    }
}
;
