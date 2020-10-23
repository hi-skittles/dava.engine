#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

using namespace DAVA;

DAVA_TESTCLASS (StringFormatTest)
{
    inline void CheckFloatFormat(const WideString& format, float32 value)
    {
        String wideFormatting = UTF8Utils::EncodeToUTF8(Format(format.c_str(), value));
        String narrowFormatting = Format(UTF8Utils::EncodeToUTF8(format).c_str(), value);
        TEST_VERIFY_WITH_MESSAGE(wideFormatting == narrowFormatting, "'" + wideFormatting + " == " + narrowFormatting + "'");
    }

    DAVA_TEST (StringTestFunction)
    {
#if !defined(__DAVAENGINE_ANDROID__)
        // Android doesn't support formatting wide string into narrow buffer
        WideString formatStr1 = L"%ls %ls";
        WideString value1 = L"test string";
        WideString value2 = L"second";
        TEST_VERIFY(Format(formatStr1.c_str(), value1.c_str(), value2.c_str()) == UTF8Utils::EncodeToWideString(Format(UTF8Utils::EncodeToUTF8(formatStr1).c_str(), value1.c_str(), value2.c_str())));
#endif
    }

    DAVA_TEST (IntegerTestFunction)
    {
        int32 value = 1234567890;
        int64 value64 = 1234567890123456789;

        TEST_VERIFY(Format(L"%i%%", value) == UTF8Utils::EncodeToWideString(Format("%i%%", value)));
        TEST_VERIFY(Format(L"%d%%", value) == UTF8Utils::EncodeToWideString(Format("%d%%", value)));
        TEST_VERIFY(Format(L"%lld%%", value64) == UTF8Utils::EncodeToWideString(Format("%lld%%", value64)));

        value *= -1;
        value64 *= -1;

        TEST_VERIFY(Format(L"%i%%", value) == UTF8Utils::EncodeToWideString(Format("%i%%", value)));
        TEST_VERIFY(Format(L"%d%%", value) == UTF8Utils::EncodeToWideString(Format("%d%%", value)));
        TEST_VERIFY(Format(L"%lld%%", value64) == UTF8Utils::EncodeToWideString(Format("%lld%%", value64)));
    }

    DAVA_TEST (FloatTestFunction)
    {
        WideString formatStr[] = {
            L"%f",
            L"%6.f", L"%5.f", L"%4.f", L"%3.f", L"%2.f", L"%1.f", L"%0.f",
            L"%.6f", L"%.5f", L"%.4f", L"%.3f", L"%.2f", L"%.1f", L"%.0f",
            L"%6.6f", L"%5.5f", L"%4.4f", L"%3.3f", L"%2.2f", L"%1.1f", L"%0.0f"
        };
        float32 values[] = {
            1234.1234f, 876.876f, 0.1234f, 0.2567f, 0.5f, 0.7543f,
            -1234.1234f, -876.876f, -0.1234f, -0.2567f, -0.5f, -0.7543f,
            12.1234f, 12.2567f, 12.5f, 12.7543f,
            -12.1234f, -12.2567f, -12.5f, -12.7543f,
            5.97391319f, -5.97391319f,
            12345.9876f, -12345.9876f, 12345.5638f, -12345.5638f,
            666.98f, -666.98f
        };
        for (auto& fmt : formatStr)
        {
            for (float32 value : values)
            {
                CheckFloatFormat(fmt.c_str(), value);
            }
        }

        CheckFloatFormat(L"%.3f", 10);
        CheckFloatFormat(L"%.0f", 12980.0f / 1000.0f);
        CheckFloatFormat(L"%.3f", 2.00671148f);
    }

    DAVA_TEST (NarrowStringFormatTest)
    {
        TEST_VERIFY(Format("%%") == "%");
        TEST_VERIFY(Format("%c", 'A') == "A");
        TEST_VERIFY(Format("%lc", L'A') == "A");
        TEST_VERIFY(Format("%s", "this is a test") == "this is a test");

        TEST_VERIFY(Format("%d", 348) == "348");
        TEST_VERIFY(Format("%2d", 348) == "348");
        TEST_VERIFY(Format("%4d", 348) == " 348");
        TEST_VERIFY(Format("%04d", 348) == "0348");
        TEST_VERIFY(Format("%-4d", 348) == "348 ");
        TEST_VERIFY(Format("%+d", 348) == "+348");
        TEST_VERIFY(Format("%d", -348) == "-348");
        TEST_VERIFY(Format("%i", 348) == "348");
        TEST_VERIFY(Format("%+i", 348) == "+348");
        TEST_VERIFY(Format("%i", -348) == "-348");

        TEST_VERIFY(Format("%X", 0x1A0D) == "1A0D");
        TEST_VERIFY(Format("%06X", 0x1A0D) == "001A0D");

        TEST_VERIFY(Format("%llX", 0x123456489ABCDEF0) == "123456489ABCDEF0");
        TEST_VERIFY(Format("%lld", 0x8000000000000000) == "-9223372036854775808");
    }

    DAVA_TEST (WideStringFormatTest)
    {
        TEST_VERIFY(Format(L"%%") == L"%");
        TEST_VERIFY(Format(L"%c", 'A') == L"A");
        TEST_VERIFY(Format(L"%lc", L'A') == L"A");
        TEST_VERIFY(Format(L"%s", "this is a test") == L"this is a test");
        TEST_VERIFY(Format(L"%S", "this is a test") == L"this is a test");
        TEST_VERIFY(Format(L"%hs", "this is a test") == L"this is a test");
        TEST_VERIFY(Format(L"%hS", "this is a test") == L"this is a test");
        TEST_VERIFY(Format(L"%ls", L"this is a test") == L"this is a test");
        TEST_VERIFY(Format(L"%lS", L"this is a test") == L"this is a test");

        TEST_VERIFY(Format(L"%d", 348) == L"348");
        TEST_VERIFY(Format(L"%2d", 348) == L"348");
        TEST_VERIFY(Format(L"%4d", 348) == L" 348");
        TEST_VERIFY(Format(L"%04d", 348) == L"0348");
        TEST_VERIFY(Format(L"%-4d", 348) == L"348 ");
        TEST_VERIFY(Format(L"%+d", 348) == L"+348");
        TEST_VERIFY(Format(L"%d", -348) == L"-348");
        TEST_VERIFY(Format(L"%i", 348) == L"348");
        TEST_VERIFY(Format(L"%+i", 348) == L"+348");
        TEST_VERIFY(Format(L"%i", -348) == L"-348");

        TEST_VERIFY(Format(L"%X", 0x1A0D) == L"1A0D");
        TEST_VERIFY(Format(L"%06X", 0x1A0D) == L"001A0D");

        TEST_VERIFY(Format(L"%llX", 0x123456489ABCDEF0) == L"123456489ABCDEF0");
        TEST_VERIFY(Format(L"%lld", 0x8000000000000000) == L"-9223372036854775808");
    }

    DAVA_TEST (VeryLongStringFormatTest)
    {
        TEST_VERIFY(Format("%s%s%s", String(100, 'A').c_str(), String(200, 'B').c_str(), String(400, 'C').c_str()).length() == 700);
        TEST_VERIFY(Format(L"%hs%hs%hs", String(100, 'A').c_str(), String(200, 'B').c_str(), String(400, 'C').c_str()).length() == 700);
        TEST_VERIFY(Format(L"%ls%ls%ls", WideString(100, 'A').c_str(), WideString(200, 'B').c_str(), WideString(400, 'C').c_str()).length() == 700);
    }

    DAVA_TEST (StringFormatAsUsedByClientTest)
    {
        // Special test case for emulating StringFormat behavior from wot.blitz client
        // Works only for wide strings
        TEST_VERIFY(StringFormatAsUsedByClient(L"", L"%s", "", std::nullptr_t()) == L"[]");
        TEST_VERIFY(StringFormatAsUsedByClient(L"", L"%s", "", L"%s", "", std::nullptr_t()) == L"[][]");
        TEST_VERIFY(StringFormatAsUsedByClient(L"", L"%ls", L"", std::nullptr_t()) == L"[]");
        TEST_VERIFY(StringFormatAsUsedByClient(L"", L"%ls", L"", L"%ls", L"", std::nullptr_t()) == L"[][]");

        TEST_VERIFY(StringFormatAsUsedByClient(L"", L"%s", "str", L"%d", 13, L"%ls", L"wstr", L"%s", "", std::nullptr_t()) == L"[str][13][wstr][]");
        TEST_VERIFY(StringFormatAsUsedByClient(L"", L"%s %ls", "str", L"wstr", L"%s", "", std::nullptr_t()) == L"[str wstr][]");
    }

    WideString StringFormatAsUsedByClient(const char16* mileStone, ...)
    {
        va_list args;
        va_start(args, mileStone);
        WideString result = StringFormatAsUsedByClientV(args);
        va_end(args);
        return result;
    }

    WideString StringFormatAsUsedByClientV(va_list & args)
    {
        WideString result;
        while (true)
        {
            const char16* fmt = va_arg(args, const char16*);
            if (fmt == nullptr)
                break;

            WideString v = FormatVL(fmt, args);
            result += L"[" + v + L"]"; // Bracket output for testing purpose
        }
        return result;
    }
}
;
