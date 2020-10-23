#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

using namespace DAVA;

struct TestStruct
{
    String description;
    WideString testString;
};

static const float32 TEST_ACCURACY = 2.f;

DAVA_TESTCLASS (TextSizeTest)
{
    Font* font = nullptr;

    TextSizeTest()
    {
        font = FTFont::Create("~res:/Fonts/DejaVuSans.ttf");
        DVASSERT(font);
    }

    ~TextSizeTest()
    {
        SafeRelease(font);
    }

    DAVA_TEST (TestFunction)
    {
        Array<TestStruct, 2> testData = { {
        { "English", L"THIS SOFTWARE IS PROVIDED BY THE DAVA, INC AND CONTRIBUTORS 'AS IS' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED" },
        { "Arabic", L"هذا البرنامج يتم توفيرها من قبل DAVA، INC والمساهمين 'كما هو' وأية ضمانات صريحة أو ضمنية، بما في ذلك على سبيل المثال لا الحصر، ضمني" },
        /*
        // Only for full unicode font
        { "Japanese", L"本ソフトウェアはDAVA、株式会社によって提供され、協力者「現状のまま」と明示しまたはその他を含む保証を、黙示、これらに限定されないが、暗黙的に指定され" },
        { "Chinese", L"本軟件提供的DAVA，公司和貢獻者“按原樣”提供任何明示或暗示的擔保，包括但不限於，暗示" },
        { "Hindi", L"इस सॉफ़्टवेयर Dava, कांग्रेस द्वारा प्रदत्त योगदानकर्ताओं 'जैसी है' और किसी प्रकट या वारंटियों, निहित है, लेकिन सीमित नहीं, निहित है" },
        */
        } };

        for (size_t k = 0; k < testData.size(); ++k)
        {
            const WideString& testString = testData[k].testString;
            Vector<float32> charSizes;
            Size2i size = font->GetStringSize(20.f, testString, &charSizes);
            float32 charsSum = 0;
            for (uint32 i = 0; i < charSizes.size(); ++i)
            {
                charsSum += charSizes[i];
            }

            TEST_VERIFY_WITH_MESSAGE(std::abs(size.dx - charsSum) < TEST_ACCURACY, testData[k].description);
        }
    }
};
