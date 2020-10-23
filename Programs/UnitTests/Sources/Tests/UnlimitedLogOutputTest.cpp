#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

using namespace DAVA;

namespace
{
const uint32 bufSize{ 4100 }; // more then 4096
const String messageEnd{ "test" };
const WideString wideMessageEnd{ L"TEST" };

String errorMessage;
}

class TestLoggerOutput : public LoggerOutput
{
public:
    TestLoggerOutput() = default;

    void Output(Logger::eLogLevel ll, const char8* text) override
    {
        std::ostringstream ostr;
        String msgFromLogger{ text };

        if (currentMessageRawSize + 1 != msgFromLogger.length())
        {
            ostr << "size of buffer do not match! bufSize == " << bufSize
                 << " msgFromLogger.length == " << msgFromLogger.length() << "\n";
        }
        const String lastNChars = msgFromLogger.substr(
        msgFromLogger.size() - (messageEnd.size() + 1), messageEnd.size());

        if (lastNChars != messageEnd)
        {
            ostr << messageEnd + " != " + lastNChars + "\n";
        }

        if (msgFromLogger.back() != '\n')
        {
            ostr << "last char should always be \\n\n";
        }

        errorMessage += ostr.str();
    }

    size_t currentMessageRawSize = 0;
};

DAVA_TESTCLASS (UnlimitedLogOutputTest)
{
    DAVA_TEST (TestFunction)
    {
        TestLoggerOutput testOutput;
        Logger::AddCustomOutput(&testOutput);

        for (auto bufSizeLocal : { 10, static_cast<int32>(bufSize), 4095, 4096, 4097 })
        {
            String str(bufSizeLocal, 'a');
            size_t startIndex = bufSizeLocal - messageEnd.size();

            for (auto c : messageEnd)
            {
                str[startIndex++] = c;
            }

            testOutput.currentMessageRawSize = bufSizeLocal;

            Logger::Info("%s", str.c_str());
        }
        Logger::RemoveCustomOutput(&testOutput);

        if (!errorMessage.empty())
        {
            Logger::Error("Error: %s", errorMessage.c_str());
        }

        TEST_VERIFY(errorMessage.empty());
    }
}
;
