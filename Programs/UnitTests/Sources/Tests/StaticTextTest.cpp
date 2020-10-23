#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

#include "Utils/StringUtils.h"
#include "Utils/UTF8Utils.h"

using namespace DAVA;

static WideString TEST_DATA = L"THIS SOFTWARE IS PROVIDED BY THE DAVA CONSULTING, LLC AND CONTRIBUTORS AS IS AND ANY EXPRESS OR IMPLIED WARRANTIES";
static struct FittingTestInfo
{
    int32 fitting;
    int32 align;
    WideString result;
} testData[] = {
    { 0, ALIGN_LEFT, L"THIS SOFTWARE" },
    { 0, ALIGN_HCENTER, L"LLC AND" },
    { 0, ALIGN_RIGHT, L"IMPLIED WARRANTIES" },
    { TextBlock::FITTING_POINTS, ALIGN_LEFT, L"..." },
    { TextBlock::FITTING_POINTS, ALIGN_HCENTER, L"..." },
    { TextBlock::FITTING_POINTS, ALIGN_RIGHT, L"..." },
    { TextBlock::FITTING_REDUCE, ALIGN_LEFT, TEST_DATA },
    { TextBlock::FITTING_REDUCE, ALIGN_HCENTER, TEST_DATA },
    { TextBlock::FITTING_REDUCE, ALIGN_RIGHT, TEST_DATA },
};
static int32 TEST_WIDTH = 400;

DAVA_TESTCLASS (StaticTextTest)
{
    UIStaticText* staticText = nullptr;
    Font* font = nullptr;

    StaticTextTest()
    {
        font = FTFont::Create("~res:/Fonts/korinna.ttf");
    }

    ~StaticTextTest()
    {
        SafeRelease(font);
    }

    void SetUp(const String& testName) override
    {
        staticText = new UIStaticText(Rect(10.f, 10.f, static_cast<float32>(TEST_WIDTH), 200.f));
        staticText->SetFont(font);
        staticText->SetFontSize(14.f);
    }

    void TearDown(const String& testName) override
    {
        SafeRelease(staticText);
    }

    void CheckLinesWidth()
    {
        Vector<WideString> strings = staticText->GetMultilineStrings();
        for (const WideString& line : strings)
        {
            Font::StringMetrics rmetrics = font->GetStringMetrics(14.f, line);
            TEST_VERIFY_WITH_MESSAGE(rmetrics.width <= TEST_WIDTH, Format("Line width %d > limit %d in line: '%s'", rmetrics.width, TEST_WIDTH, UTF8Utils::EncodeToUTF8(line).c_str()));
        }
    }

    DAVA_TEST (SplitByWords)
    {
        staticText->SetText(TEST_DATA);
        staticText->SetMultiline(true);

        CheckLinesWidth();
    }

    DAVA_TEST (SplitBySymbols)
    {
        staticText->SetText(TEST_DATA);
        staticText->SetMultiline(true, true);

        CheckLinesWidth();
    }

    DAVA_TEST (SplitByWordsWithNewLine)
    {
        staticText->SetText(L"THIS SOFTWARE IS PROVIDED BY\nTHE DAVA CONSULTING,\nLLC AND\nCONTRIBUTORS AS IS AND ANY EXPRESS OR IMPLIED WARRANTIES");
        staticText->SetMultiline(true);

        CheckLinesWidth();
    }

    DAVA_TEST (SplitBySymbolsWithNewLine)
    {
        staticText->SetText(L"THIS SOFTWARE IS PROVIDED BY\nTHE DAVA CONSULTING,\nLLC AND\nCONTRIBUTORS AS IS AND ANY EXPRESS OR IMPLIED WARRANTIES");
        staticText->SetMultiline(true, true);

        CheckLinesWidth();
    }

    DAVA_TEST (SplitAndTrimTest)
    {
        staticText->SetText(L"  THIS SOFTWARE IS   \u00A0   PROVIDED BY   \u00A0   \n  THE DAVA CONSULTING\n LLC AND CONTRIBUTORS AS IS AND ANY EXPRESS OR IMPLIED WARRANTIES");
        staticText->SetMultiline(true);

        CheckLinesWidth();
    }

    DAVA_TEST (CleanLineTest)
    {
        WideString test1 = StringUtils::RemoveNonPrintable(L"THIS SOFTWARE\u00A0IS PROV\u200BIDED BY\n THE DAVA CONS\u200BULTING\n LLC");
        WideString test2 = StringUtils::RemoveNonPrintable(L"THIS\tSOFTWARE IS\tPROVIDED BY\nTHE DAVA CONSULTING\nLLC");
        WideString test3 = StringUtils::RemoveNonPrintable(L"THIS\tSOFTWARE IS\tPROVIDED BY\nTHE DAVA CONSULTING\nLLC", 1);
        WideString test4 = StringUtils::RemoveNonPrintable(L"THIS\tSOFTWARE IS\tPROVIDED BY\nTHE DAVA CONSULTING\nLLC", 4);

        WideString out1 = L"THIS SOFTWARE IS PROVIDED BY THE DAVA CONSULTING LLC";
        WideString out2 = L"THIS\tSOFTWARE IS\tPROVIDED BYTHE DAVA CONSULTINGLLC";
        WideString out3 = L"THIS SOFTWARE IS PROVIDED BYTHE DAVA CONSULTINGLLC";
        WideString out4 = L"THIS    SOFTWARE IS    PROVIDED BYTHE DAVA CONSULTINGLLC";

        TEST_VERIFY(test1 == out1);
        TEST_VERIFY(test2 == out2);
        TEST_VERIFY(test3 == out3);
        TEST_VERIFY(test4 == out4);
    }

    DAVA_TEST (TestFitting)
    {
        staticText->SetText(TEST_DATA);
        staticText->SetMultiline(false);
        for (const auto& data : testData)
        {
            staticText->SetFittingOption(data.fitting);
            staticText->SetTextAlign(data.align);

            const WideString& result = staticText->GetVisualText();
            TEST_VERIFY_WITH_MESSAGE(result.find(data.result) != String::npos, Format("Line '%s' doesn't contain '%s'", UTF8Utils::EncodeToUTF8(result).c_str(), UTF8Utils::EncodeToUTF8(data.result).c_str()));

            Font::StringMetrics rmetrics = font->GetStringMetrics(staticText->GetTextBlock()->GetRenderSize(), result);
            TEST_VERIFY_WITH_MESSAGE(rmetrics.width <= TEST_WIDTH, Format("Line width %d > limit %d in line: %s", rmetrics.width, TEST_WIDTH, UTF8Utils::EncodeToUTF8(result).c_str()));
        }
    }
};
