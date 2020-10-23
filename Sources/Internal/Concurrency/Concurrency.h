#pragma once

// All concurrency subsystem includes

#include "Concurrency/Atomic.h"
#include "Concurrency/ConcurrentObject.h"
#include "Concurrency/ConditionVariable.h"
#include "Concurrency/LockGuard.h"
#include "Concurrency/Mutex.h"
#include "Concurrency/Semaphore.h"
#include "Concurrency/Spinlock.h"
#include "Concurrency/SyncBarrier.h"
#include "Concurrency/Thread.h"
#include "Concurrency/ThreadLocalPtr.h"

//TODO: uncomment this include in client
#include "Concurrency/PosixThreads.h"
