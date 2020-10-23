#pragma once

#include "Base/Platform.h"
#ifndef __DAVAENGINE_WINDOWS__
#include <pthread.h>
#else

//mimic to some posix threads api
//No cancellations!

namespace DAVA
{
using pthread_condattr_t = void;
using pthread_cond_t = CONDITION_VARIABLE;

struct pthread_mutexattr_t
{
    bool isRecursive;
};

struct pthread_mutex_t
{
    pthread_mutexattr_t attributes;
    CRITICAL_SECTION critical_section;
};

#define PTHREAD_COND_INITIALIZER {0}
const int PTHREAD_MUTEX_RECURSIVE = 1;

int pthread_cond_init(pthread_cond_t* cv, const pthread_condattr_t*);
int pthread_cond_wait(pthread_cond_t* cv, pthread_mutex_t* external_mutex);
int pthread_cond_signal(pthread_cond_t* cv);
int pthread_cond_broadcast(pthread_cond_t* cv);
int pthread_cond_destroy(pthread_cond_t* cond);

int pthread_mutexattr_init(pthread_mutexattr_t* attr);
int pthread_mutexattr_settype(pthread_mutexattr_t* attr, int type);
int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* mutexattr);
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_trylock(pthread_mutex_t* mutex);
int pthread_mutex_unlock(pthread_mutex_t* mutex);
int pthread_mutex_destroy(pthread_mutex_t* mutex);
};

#endif //  !__DAVAENGINE_WINDOWS__
