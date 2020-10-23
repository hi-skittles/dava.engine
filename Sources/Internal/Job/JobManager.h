#pragma once

#include "Base/BaseTypes.h"
#include "Concurrency/Atomic.h"
#include "Concurrency/Mutex.h"
#include "Concurrency/Semaphore.h"
#include "Concurrency/Thread.h"
#include "Functional/Function.h"
#include "Job/JobQueue.h"

namespace DAVA
{
class Engine;
class JobThread;
class JobManager
{
public:
    /*! Available types of main-thread job. */
    enum eMainJobType
    {
        JOB_MAIN = 0, ///< Run only in the main thread. If job is created from the main thread, function will be run immediately.
        JOB_MAINLAZY, ///< Run only in the main thread. If job is created from the main thread, function will be run on next update.
        JOB_MAINBG, ///< Run in the main or background thread. !!!!!!! TODO: isn't implemented yet
    };

public:
    JobManager(Engine* e);
    virtual ~JobManager();

    /*! This function should be called periodically from the main thread. All main-thread jobs added to the queue
        will be performed inside this function. 
    */
    void Update(float32 frameDelta = 0.0f);

    /*! Add function to execute in the main-thread.
		\param [in] fn Function to execute.
		\param [in] mainJobType Type of execution. See ::eMainJobType for detailed description.
        \return id Created job id. This id can be used to wait until this job finished.
	*/
    uint32 CreateMainJob(const Function<void()>& fn, eMainJobType mainJobType = JOB_MAIN);

    /*! Wait for the main-thread jobs, that were added from other thread with the given ID. 
		\param [in] invokerThreadId Thread ID. By default it is 0, which means that current thread ID will be taken.
	*/
    void WaitMainJobs(Thread::Id invokerThreadId = Thread::Id());

    void WaitMainJobID(uint32 mainJobID);

    /*! Check in there are some main-thread jobs in the queue, that were added from thread with the given ID.
		\param [in] invokerThreadId Thread ID. By default it is 0, which means that current thread ID will be taken.
		\return Return true if there are some jobs, otherwise false.
	*/
    bool HasMainJobs(Thread::Id invokerThreadId = Thread::Id());

    bool HasMainJobID(uint32 mainJobID);

    /*! Returns the number of available worker-threads. */
    uint32 GetWorkersCount() const;

    /*! Add function to execute in the worker-thread.
		\param [in] fn Function to execute.
	*/
    void CreateWorkerJob(const Function<void()>& fn);

    /*! Wait until all worker-thread jobs are executed. */
    void WaitWorkerJobs();

    /*!  Check in there are some not executed worker-thread jobs.
		\return Return true if there are some jobs, otherwise false.
	*/
    bool HasWorkerJobs();

protected:
    struct MainJob
    {
        MainJob()
            : id(0)
            , type(JOB_MAIN)
            , invokerThreadId(Thread::Id())
        {
        }

        uint32 id;
        eMainJobType type;
        Thread::Id invokerThreadId;

        Function<void()> fn;
    };

    Engine* engine = nullptr;
    Atomic<uint32> mainJobIDCounter;
    uint32 mainJobLastExecutedID;

    Mutex mainQueueMutex;
    Mutex mainCVMutex;
    Deque<MainJob> mainJobs;
    ConditionVariable mainCV;
    MainJob curMainJob;

    Semaphore workerDoneSem;
    JobQueueWorker workerQueue;
    Vector<JobThread*> workerThreads;
};
}
