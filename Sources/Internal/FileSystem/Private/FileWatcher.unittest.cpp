#include "UnitTests/UnitTests.h"

#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_MACOS__)
#include <Base/String.h>
#include <Concurrency/Thread.h>
#include <Engine/Engine.h>
#include <Engine/EngineContext.h>
#include <FileSystem/FilePath.h>
#include <FileSystem/FileSystem.h>
#include <FileSystem/FileWatcher.h>
#include <Functional/Function.h>
#include <Logger/Logger.h>
#include <Time/SystemTimer.h>
#include <Debug/DebuggerDetection.h>

DAVA_TESTCLASS (FileWatcherTests)
{
    struct ExpectedEvent
    {
        DAVA::String path;
        DAVA::FileWatcher::eWatchEvent event;
    };

    struct TestEvent
    {
        DAVA::Vector<ExpectedEvent> e;
        DAVA::Function<void()> fn;
    };

    ExpectedEvent MakeExpectedEvent(const DAVA::String& path, DAVA::FileWatcher::eWatchEvent e)
    {
        ExpectedEvent ev;
        ev.path = testFolder.GetAbsolutePathname() + path;
        ev.event = e;
        return ev;
    }

    FileWatcherTests()
    {
        const DAVA::EngineContext* ctx = DAVA::GetEngineContext();
        testFolder = ctx->fileSystem->GetTempDirectoryPath();
        testFolder.MakeDirectoryPathname();
        testFolder += "FileWatcherTest/";

        ctx->fileSystem->DeleteDirectory(testFolder, true);
        ctx->fileSystem->CreateDirectory(testFolder);
        DAVA::Thread::Sleep(100);

        startMs = DAVA::SystemTimer::GetMs();

        watcher = new DAVA::FileWatcher();
    }

    ~FileWatcherTests()
    {
        delete watcher;
        DAVA::GetEngineContext()->fileSystem->DeleteDirectory(testFolder, true);
    }

    void Step1()
    {
        DAVA::FileSystem* fs = DAVA::GetEngineContext()->fileSystem;
        fs->CreateDirectory(testFolder + "A");
        fs->CreateDirectory(testFolder + "B");

        DAVA::ScopedPtr<DAVA::File> aaFile(DAVA::File::Create(testFolder + "A/aa", DAVA::File::CREATE | DAVA::File::WRITE));
        aaFile->WriteString("Hello", false);
        aaFile->Flush();

        DAVA::ScopedPtr<DAVA::File> bbFile(DAVA::File::Create(testFolder + "B/bb", DAVA::File::CREATE | DAVA::File::WRITE));
        bbFile->WriteString("world", false);
        bbFile->Flush();
    }

    void Step2()
    {
        DAVA::FileSystem* fs = DAVA::GetEngineContext()->fileSystem;
        fs->MoveFile(testFolder + "A/aa", testFolder + "A/a");
    }

    void Step3()
    {
        DAVA::FileSystem* fs = DAVA::GetEngineContext()->fileSystem;
        fs->DeleteDirectory(testFolder + "B/");
    }

    void Step4()
    {
        DAVA::Thread::Sleep(1000);
        DAVA::ScopedPtr<DAVA::File> aaFile(DAVA::File::Create(testFolder + "A/a", DAVA::File::OPEN | DAVA::File::WRITE | DAVA::File::APPEND));
        aaFile->WriteString(" world", false);
        aaFile->Flush();
    }

    DAVA_TEST (FileCreationTest)
    {
        using namespace DAVA;
        watcher->Add(testFolder.GetAbsolutePathname(), true);

        {
            TestEvent ev;
            ev.e.push_back(MakeExpectedEvent("A", FileWatcher::FILE_CREATED));
            ev.e.push_back(MakeExpectedEvent("B", FileWatcher::FILE_CREATED));
            ev.e.push_back(MakeExpectedEvent("A/aa", FileWatcher::FILE_CREATED));
            ev.e.push_back(MakeExpectedEvent("B/bb", FileWatcher::FILE_CREATED));
#if defined(__DAVAENGINE_WIN32__)
            ev.e.push_back(MakeExpectedEvent("B/bb", FileWatcher::FILE_MODIFIED));
            ev.e.push_back(MakeExpectedEvent("A/aa", FileWatcher::FILE_MODIFIED));
#endif //__DAVAENGINE_WIN32__
            ev.fn = MakeFunction(this, &FileWatcherTests::Step2);
            queue.push_back(ev);
        }

        {
            TestEvent ev;
            ev.e.push_back(MakeExpectedEvent("A/aa", FileWatcher::FILE_REMOVED));
            ev.e.push_back(MakeExpectedEvent("A/a", FileWatcher::FILE_CREATED));
#if defined(__DAVAENGINE_WIN32__)
            ev.e.push_back(MakeExpectedEvent("A", FileWatcher::FILE_MODIFIED));
            ev.e.push_back(MakeExpectedEvent("A", FileWatcher::FILE_MODIFIED));
#endif //__DAVAENGINE_WIN32__
            ev.fn = MakeFunction(this, &FileWatcherTests::Step3);
            queue.push_back(ev);
        }

        {
            TestEvent ev;
            ev.e.push_back(MakeExpectedEvent("B/bb", FileWatcher::FILE_REMOVED));
            ev.e.push_back(MakeExpectedEvent("B", FileWatcher::FILE_REMOVED));
#if defined(__DAVAENGINE_WIN32__)
            ev.e.push_back(MakeExpectedEvent("B", FileWatcher::FILE_MODIFIED));
            ev.e.push_back(MakeExpectedEvent("B", FileWatcher::FILE_MODIFIED));
#endif //__DAVAENGINE_WIN32__
            ev.fn = MakeFunction(this, &FileWatcherTests::Step4);
            queue.push_back(ev);
        }

        {
            TestEvent ev;
            ev.e.push_back(MakeExpectedEvent("A/a", FileWatcher::FILE_MODIFIED));
            ev.fn = [] {};
            queue.push_back(ev);
        }

        watcher->onWatchersChanged.Connect([this](const DAVA::String& filename, DAVA::FileWatcher::eWatchEvent e) {
            DAVA::Logger::Info("FileWatcher event %s : %d", filename.c_str(), static_cast<int>(e));
            TEST_VERIFY(DAVA::Thread::IsMainThread() == true);
            TestEvent& expectation = queue.front();

            bool found = false;
            auto iter = expectation.e.begin();
            while (iter != expectation.e.end())
            {
                if (iter->event == e && iter->path == filename)
                {
                    found = true;
                    iter = expectation.e.erase(iter);
                    break;
                }

                ++iter;
            }

            TEST_VERIFY(found == true);

            if (expectation.e.empty())
            {
                expectation.fn();
                queue.pop_front();
            }
        });

        Step1();
    }

    bool TestComplete(const DAVA::String& testName) const
    {
        if (DAVA::IsDebuggerPresent() == false)
        {
            if ((DAVA::SystemTimer::GetMs() - startMs) < 500)
            {
                TEST_VERIFY((DAVA::SystemTimer::GetMs() - startMs) < 500);
                return true;
            }
        }
        return queue.empty();
    }

    void TearDown(const DAVA::String& testName)
    {
        watcher->onWatchersChanged.DisconnectAll();
    }

    DAVA::Deque<TestEvent> queue;
    DAVA::FileWatcher* watcher = nullptr;
    DAVA::FilePath testFolder;
    DAVA::int64 startMs = 0;
};

#endif
