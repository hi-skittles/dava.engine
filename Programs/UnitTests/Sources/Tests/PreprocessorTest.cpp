#include "UnitTests/UnitTests.h"
#include "FileSystem/File.h"
#include "Logger/Logger.h"
#include "Render/RHI/Common/PreProcessor.h"
#include "Render/RHI/Common/rhi_Utils.h"

static float EV_OneMore(float x)
{
    return x + 1.0f;
}

void DumpAndCompareBytes(const char* ptr0, size_t size0, const char* ptr1, size_t size1)
{
    size_t maxSize = std::max(size0, size1);
    size_t pos = 0;
    while (pos < maxSize)
    {
        char line[256] = {};
        size_t len = 8;

        DAVA::int32 linePos = 6;
        for (DAVA::uint32 i = 0; i < len; ++i)
        {
            char c = (pos + i < size0) ? *(ptr0 + pos + i) : 0;
            linePos += sprintf(line + linePos, "%02x ", c);
        }

        linePos += sprintf(line + linePos, "| ");

        for (DAVA::uint32 i = 0; i < len; ++i)
        {
            char c = (pos + i < size1) ? *(ptr1 + pos + i) : 0;
            linePos += sprintf(line + linePos, "%02x ", c);
        }

        linePos += sprintf(line + linePos, ">>> ");

        for (DAVA::uint32 i = 0; i < len; ++i)
        {
            DAVA::uint8 c = static_cast<DAVA::uint8>((pos + i < size0) ? *(ptr0 + pos + i) : 0);
            linePos += sprintf(line + linePos, "%c", ((c >= 0x20) && (c <= 0x7f)) ? static_cast<char>(c) : '.');
        }

        linePos += sprintf(line + linePos, " | ");

        for (DAVA::uint32 i = 0; i < len; ++i)
        {
            DAVA::uint8 c = static_cast<DAVA::uint8>((pos + i < size1) ? *(ptr1 + pos + i) : 0);
            linePos += sprintf(line + linePos, "%c", ((c >= 0x20) && (c <= 0x7f)) ? static_cast<char>(c) : '.');
        }

        bool same = memcmp(ptr0 + pos, ptr1 + pos, len) == 0;
        sprintf(line, "%s", same ? "GOOD:" : "FAIL:");
        line[5] = ' ';

        DAVA::Logger::Error("%s", line);
        pos += len;
    }
}

static bool ReadTextData(const char* fileName, std::vector<char>* data)
{
    bool success = false;
    DAVA::ScopedPtr<DAVA::File> file(DAVA::File::Create(fileName, DAVA::File::READ | DAVA::File::OPEN));
    if (file.get() != nullptr)
    {
        data->resize(static_cast<size_t>(file->GetSize()));
        file->Read(data->data(), static_cast<DAVA::uint32>(file->GetSize()));
        success = true;
    }
    return success;
}

DAVA_TESTCLASS (PreprocessorTest)
{
    static bool CompareStringBuffers();

    DAVA_TEST (TestExpressionEvaluator)
    {
        DAVA::ExpressionEvaluator ev;
        struct
        {
            const char* expr;
            const float result;
        } data[] =
        {
          { "1 > 0", 1.0f },
          { "1 < 0", 0.0f },
          { "2 < 4", 1.0f },
          { "2 > 4", 0.0f },

          { "1 >= 0", 1.0f },
          { "1 <= 0", 0.0f },
          { "2 <= 4", 1.0f },
          { "2 >= 4", 0.0f },

          { "2 <= 2", 1.0f },
          { "2 >= 2", 1.0f },

          { "2+2", 4.0f },
          { "bla+7", 20.0f },
          { "(5+3) / (3-1)", 4.0f },
          { "3 + ((1+7)/2) + 1", 8.0f },
          { "SHADING == SHADING_PERVERTEX", 1.0f },
          { "SHADING != SHADING_NONE", 1.0f },
          { "LIGHTING_ENABLED", 1.0f },
          { "!DARKNESS_DISABLED", 1.0f },
          { "!DARKNESS_DISABLED && SHADING != SHADING_NONE", 1.0f },
          { "LIGHTING_ENABLED || !DARKNESS_DISABLED", 1.0f },
          { "defined DARKNESS_DISABLED", 1.0f },
          { "!defined DARKNESS_DISABLED", 0.0f },
          { "!defined RANDOM_BULLSHIT", 1.0f },
          { "defined RANDOM_BULLSHIT", 0.0f },
          { "defined(RANDOM_BRACED_BULLSHIT)", 0.0f },
          { "!defined(RANDOM_BRACED_BULLSHIT)", 1.0f },
          { "SHADING != SHADING_NONE  &&  !defined RANDOM_BULLSHIT", 1.0f },
          { "!(LIGHTING_ENABLED)", 0.0f },
          { "!(LIGHTING_ENABLED && DARKNESS_ENABLED)", 1.0f },
          { "abs(-7)", 7.0f },
          { "one_more(4)", 5.0f },
          { "one_more(one_more(4))", 6.0f }
        };

        ev.SetVariable("bla", 13);
        ev.SetVariable("SHADING_NONE", 0);
        ev.SetVariable("SHADING_PERVERTEX", 1);
        ev.SetVariable("SHADING_PERPIXEL", 2);
        ev.SetVariable("SHADING", 1);
        ev.SetVariable("LIGHTING_ENABLED", 1);
        ev.SetVariable("DARKNESS_ENABLED", 0);
        ev.SetVariable("DARKNESS_DISABLED", 0);
        DAVA::ExpressionEvaluator::RegisterCommonFunctions();
        DAVA::ExpressionEvaluator::RegisterFunction("one_more", &EV_OneMore);

        for (DAVA::uint32 i = 0; i != countof(data); ++i)
        {
            float res = 0;
            bool success = ev.Evaluate(data[i].expr, &res);

            TEST_VERIFY(success);
            TEST_VERIFY(FLOAT_EQUAL(res, data[i].result))
        }

        const char* err_expr[] =
        {
          "BULLSHIT+2"
        };

        for (DAVA::uint32 i = 0; i != countof(err_expr); ++i)
        {
            float res = 0;
            bool success = ev.Evaluate(err_expr[i], &res);
            char err[256] = "";

            TEST_VERIFY(!success);
            ev.GetLastError(err, countof(err));
            DAVA::Logger::Info("(expected) expr.eval error : %s", err);
        }
    }

    DAVA_TEST (TestPreprocessor)
    {
        struct
        {
            const char* inputFileName;
            const char* resultFileName;
        } test[] =
        {
          { "DefineInDefine-input.preproc", "DefineInDefine-output.preproc" },
          { "Multiple-Defines-input.preproc", "Multiple-Defines-output.preproc" },
          { "PurrfectTest-input.preproc", "PurrfectTest-output.preproc" },
          { "00-input.preproc", "00-output.preproc" },
          { "01-input.preproc", "01-output.preproc" },
          { "02-input.preproc", "02-output.preproc" },
          { "03-input.preproc", "03-output.preproc" },
          { "04-input.preproc", "04-output.preproc" },
          { "05-input.preproc", "05-output.preproc" },
          { "06-input.preproc", "06-output.preproc" },
          { "07-input.preproc", "07-output.preproc" },
          { "08-input.preproc", "08-output.preproc" },
          { "09-input.preproc", "09-output.preproc" },
          { "10-input.preproc", "10-output.preproc" },
          { "11-input.preproc", "11-output.preproc" },
          { "12-input.preproc", "12-output.preproc" },
          { "CC01-input.preproc", "CC01-output.preproc" },
          { "CC02-input.preproc", "CC02-output.preproc" },
          { "CC03-input.preproc", "CC03-output.preproc" },
          { "CC04-input.preproc", "CC04-output.preproc" },
          { "CC05-input.preproc", "CC05-output.preproc" },
          { "CC06-input.preproc", "CC06-output.preproc" },
          { "CC07-input.preproc", "CC07-output.preproc" },
          { "CC08-input.preproc", "CC08-output.preproc" },
          { "CC09-input.preproc", "CC09-output.preproc" },
          { "CC10-input.preproc", "CC10-output.preproc" }
        };
        static const char* BaseDir = "~res:/TestData/PreProcessor";

        class TestFileCallback : public DAVA::PreProc::FileCallback
        {
        public:
            TestFileCallback(const char* base_dir)
                : _base_dir(base_dir)
                , _in(nullptr)
            {
            }

            bool Open(const char* file_name) override
            {
                char fname[2048];
                Snprintf(fname, countof(fname), "%s/%s", _base_dir, file_name);
                _in = DAVA::File::Create(fname, DAVA::File::READ | DAVA::File::OPEN);
                return _in != nullptr;
            }

            void Close() override
            {
                DVASSERT(_in != nullptr);
                _in->Release();
                _in = nullptr;
            }

            DAVA::uint32 Size() const override
            {
                return (_in) ? unsigned(_in->GetSize()) : 0;
            }

            DAVA::uint32 Read(DAVA::uint32 max_sz, void* dst) override
            {
                return (_in) ? _in->Read(dst, max_sz) : 0;
            }

        private:
            DAVA::File* _in;
            const char* const _base_dir;
        };

        for (int i = 0; i != countof(test); ++i)
        {
            DAVA::Logger::Info("pre-proc test %i  \"%s\"...", i, test[i].inputFileName);

            TestFileCallback fc(BaseDir);
            DAVA::PreProc pp(&fc);
            std::vector<char> output;
            std::vector<char> expected;
            char fname[2048];
            Snprintf(fname, countof(fname), "%s/%s", BaseDir, test[i].resultFileName);
            TEST_VERIFY(ReadTextData(fname, &expected));
            TEST_VERIFY(pp.ProcessFile(test[i].inputFileName, &output));

            const char* actual_data = output.data();
            const char* expected_data = expected.data();
            bool size_match = output.size() == expected.size();
            bool content_match = strncmp(expected_data, actual_data, output.size()) == 0;
            if (!size_match || !content_match)
            {
                DAVA::Logger::Error("Preprocessor test failed:");
                DumpAndCompareBytes(actual_data, output.size(), expected_data, expected.size());
            }
            TEST_VERIFY(size_match);
            TEST_VERIFY(content_match);

            DAVA::Logger::Info("  OK");
        }

        const char* err_test[] =
        {
          "UnterminatedComment-input.preproc",
          "E01-input.preproc",
          "E02-input.preproc",
          "E03-input.preproc",
          "E04-input.preproc"
        };

        for (int i = 0; i != countof(err_test); ++i)
        {
            DAVA::Logger::Info("pre-proc error-test %i  \"%s\"...", i, err_test[i]);

            TestFileCallback fc(BaseDir);
            DAVA::PreProc pp(&fc);
            std::vector<char> output;

            TEST_VERIFY(pp.ProcessFile(err_test[i], &output) == false);
            DAVA::Logger::Info("  OK");
        }

        DAVA::Logger::Info("pre-proc tests PASSED");
    }
};
