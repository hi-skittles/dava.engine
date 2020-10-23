#include "Engine/EngineContext.h"

#include "Concurrency/Mutex.h"
#include "Concurrency/Thread.h"
#include "Concurrency/LockGuard.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast" // suppress warnings from openssl headers
#endif

#define CURL_STATICLIB
#include <curl/curl.h>
#include <openssl/crypto.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace DAVA
{
namespace Context
{
static Mutex curlInitializeMut;
static int counterCurlInitialization{ 0 };
static Mutex* ssl_mutexes = nullptr;

void SslMultiThreadInit()
{
    ssl_mutexes = new Mutex[CRYPTO_num_locks()];

    CRYPTO_set_id_callback([]()
                           {
                               unsigned long id = static_cast<unsigned long>(Thread::GetCurrentId());
                               return id;
                           });

    CRYPTO_set_locking_callback([](int mode, int n, const char* file, int line)
                                {
                                    if (mode & CRYPTO_LOCK)
                                    {
                                        ssl_mutexes[n].Lock();
                                    }
                                    else
                                    {
                                        ssl_mutexes[n].Unlock();
                                    }
                                });
}

void SslMultiThreadDeinit()
{
    CRYPTO_set_id_callback(nullptr);
    CRYPTO_set_locking_callback(nullptr);

    delete[] ssl_mutexes;
    ssl_mutexes = nullptr;
}

void CurlGlobalInit()
{
    LockGuard<Mutex> lock(curlInitializeMut);
    if (counterCurlInitialization == 0)
    {
        // https://curl.haxx.se/libcurl/c/curl_global_init.html
        CURLcode code = curl_global_init(CURL_GLOBAL_ALL);
        if (CURLE_OK != code)
        {
            StringStream ss;
            ss << "curl_global_init failed: CURLcode == " << code << std::endl;
            DAVA_THROW(Exception, ss.str());
        }

        // https://curl.haxx.se/libcurl/c/threadsafe.html
        SslMultiThreadInit();
    }
    ++counterCurlInitialization;
}
void CurlGlobalDeinit()
{
    LockGuard<Mutex> lock(curlInitializeMut);
    if (counterCurlInitialization > 0)
    {
        --counterCurlInitialization;
    }
    else
    {
        DVASSERT(counterCurlInitialization >= 0);
    }

    if (counterCurlInitialization == 0)
    {
        curl_global_cleanup();
        SslMultiThreadDeinit();
    }
}
} // namespace Context
} // namespace DAVA
