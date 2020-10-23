#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"
#include "Engine/Engine.h"

using namespace DAVA;

DAVA_TESTCLASS (LoggerFileTest)
{
    DAVA_TEST (TestFunction)
    {
        const uint32 logCutSize = 80;
        const String filename("TestLogFile.txt");
        const FilePath logFilePath(Logger::GetLogPathForFilename(filename));

        {
            ScopedPtr<File> log(File::Create(logFilePath, File::CREATE | File::WRITE));
        }

        Logger* logger = GetEngineContext()->logger;
        logger->SetMaxFileSize(logCutSize);

        for (uint32 i = 0; i < 10; ++i)
        {
            if (i > 0)
            {
                ScopedPtr<File> log(File::Create(logFilePath, File::OPEN | File::READ));
                TEST_VERIFY(log);
                uint64 size = log->GetSize();
                // log could have any size from last session
                TEST_VERIFY(logCutSize < size);
            }

            // choult to cut file to logCutSize
            logger->SetLogFilename(filename);

            {
                ScopedPtr<File> log(File::Create(logFilePath, File::OPEN | File::READ));
                TEST_VERIFY(log);
                uint64 size = log->GetSize();
                // current session should start from last logCutSize bytes of prev session.
                TEST_VERIFY(logCutSize >= size);
            }

            // fill log
            Logger::Debug(Format("%d ==", i).c_str());
            for (uint32 j = 0; j < logCutSize; ++j)
            {
                Logger::Debug(Format("%d++", j).c_str());
            }
        }
    }
}
;
