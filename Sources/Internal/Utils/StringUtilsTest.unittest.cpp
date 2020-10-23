#include "Utils/StringUtils.h"

#include <UnitTests/UnitTests.h>

using namespace DAVA;

DAVA_TESTCLASS (StringUtilsTest)
{
    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(DavaFramework)
    DECLARE_COVERED_FILES("StringUtils.cpp")
    END_FILES_COVERED_BY_TESTS()

    // Utils::Trim
    DAVA_TEST (TrimTest)
    {
        TEST_VERIFY(StringUtils::Trim(String("abc")) == "abc");
        TEST_VERIFY(StringUtils::Trim(String("   abc")) == "abc");
        TEST_VERIFY(StringUtils::Trim(String("abc     ")) == "abc");
        TEST_VERIFY(StringUtils::Trim(String("   abc    ")) == "abc");
        TEST_VERIFY(StringUtils::Trim(String("\tabc\t")) == "abc");
        TEST_VERIFY(StringUtils::Trim(String("    ")) == "");
        TEST_VERIFY(StringUtils::Trim(String("\t\t")) == "");

        TEST_VERIFY(StringUtils::TrimLeft(String("abc")) == "abc");
        TEST_VERIFY(StringUtils::TrimLeft(String("   abc")) == "abc");
        TEST_VERIFY(StringUtils::TrimLeft(String("abc     ")) == "abc     ");
        TEST_VERIFY(StringUtils::TrimLeft(String("   abc    ")) == "abc    ");
        TEST_VERIFY(StringUtils::TrimLeft(String("\tabc\t")) == "abc\t");
        TEST_VERIFY(StringUtils::TrimLeft(String("    ")) == "");
        TEST_VERIFY(StringUtils::TrimLeft(String("\t\t")) == "");

        TEST_VERIFY(StringUtils::TrimRight(String("abc")) == "abc");
        TEST_VERIFY(StringUtils::TrimRight(String("   abc")) == "   abc");
        TEST_VERIFY(StringUtils::TrimRight(String("abc     ")) == "abc");
        TEST_VERIFY(StringUtils::TrimRight(String("   abc    ")) == "   abc");
        TEST_VERIFY(StringUtils::TrimRight(String("\tabc\t")) == "\tabc");
        TEST_VERIFY(StringUtils::TrimRight(String("    ")) == "");
        TEST_VERIFY(StringUtils::TrimRight(String("\t\t")) == "");
    }

    DAVA_TEST (StartsWithTest)
    {
        TEST_VERIFY(StringUtils::StartsWith("abcdef", "abc") == true);
        TEST_VERIFY(StringUtils::StartsWith("abc", "abc") == true);
        TEST_VERIFY(StringUtils::StartsWith("ab", "abc") == false);
        TEST_VERIFY(StringUtils::StartsWith("", "abc") == false);
        TEST_VERIFY(StringUtils::StartsWith("abcdef", "ac") == false);
    }

    DAVA_TEST (ContainsIgnoreCaseTest)
    {
        using StringUtils::ContainsIgnoreCase;
        String str1, str2;
        str1 = "TEST";
        str2 = "test";
        TEST_VERIFY(ContainsIgnoreCase(str1, str2) == true);

        str1 = "someTEST";
        str2 = "test";
        TEST_VERIFY(ContainsIgnoreCase(str1, str2) == true);

        str1 = "TESTsome";
        str2 = "test";
        TEST_VERIFY(ContainsIgnoreCase(str1, str2) == true);

        str1 = "TEST";
        str2 = "TEST";
        TEST_VERIFY(ContainsIgnoreCase(str1, str2) == true);

        str1 = "test";
        str2 = "test";
        TEST_VERIFY(ContainsIgnoreCase(str1, str2) == true);

        str1 = "TEST";
        str2 = "not found";
        TEST_VERIFY(ContainsIgnoreCase(str1, str2) == false);
    }

    DAVA_TEST (RemoveNonPrintableTest)
    {
        using StringUtils::RemoveNonPrintable;
        WideString str1;
        str1 = L"\ta\tbc\t";
        TEST_VERIFY(str1 == RemoveNonPrintable(str1));

        WideString str2;
        str1 = L"abc";
        str2 = L"a\tbc";
        TEST_VERIFY(str1 == RemoveNonPrintable(str2, 0));

        str1 = L"abc";
        str2 = L"\tabc";
        TEST_VERIFY(str1 == RemoveNonPrintable(str2, 0));

        str1 = L"abc";
        str2 = L"abc\t";
        TEST_VERIFY(str1 == RemoveNonPrintable(str2, 0));

        str1 = L"abc";
        str2 = L"\t\ta\tbc\t\t";
        TEST_VERIFY(str1 == RemoveNonPrintable(str2, 0));

        str1 = L" abc";
        str2 = L"\tabc";
        TEST_VERIFY(str1 == RemoveNonPrintable(str2, 1));

        str1 = L"abc ";
        str2 = L"abc\t";
        TEST_VERIFY(str1 == RemoveNonPrintable(str2, 1));

        str1 = L"  abc  ";
        str2 = L"\tabc\t";
        TEST_VERIFY(str1 == RemoveNonPrintable(str2, 2));

        str1 = L" abc";
        str2 = L"*abc";
        str2[0] = 0x00A0; // non-breaking space
        TEST_VERIFY(str1 == RemoveNonPrintable(str2));
    }

    DAVA_TEST (CompareIgnoreCaseTest)
    {
        TEST_VERIFY(StringUtils::CompareIgnoreCase(String("abc"), String("ABC")));
        TEST_VERIFY(StringUtils::CompareIgnoreCase(String(" abc "), String(" ABC ")));
        TEST_VERIFY(StringUtils::CompareIgnoreCase(String("ABC"), String("abc")));
        TEST_VERIFY(StringUtils::CompareIgnoreCase(String("ABC"), String("ABC")));
        TEST_VERIFY(StringUtils::CompareIgnoreCase(String("abc"), String("abc")));
        TEST_VERIFY(StringUtils::CompareIgnoreCase(String("ABCdef"), String("abcDEF")));
        TEST_VERIFY(!StringUtils::CompareIgnoreCase(String("ABC"), String("CDE")));
        TEST_VERIFY(!StringUtils::CompareIgnoreCase(String("abc"), String("abcdef")));
        TEST_VERIFY(!StringUtils::CompareIgnoreCase(String("abcdef"), String("abc")));
        TEST_VERIFY(!StringUtils::CompareIgnoreCase(String(" abc "), String("abc")));
    }

    // Utils::ToUpperCase
    DAVA_TEST (UpperCaseTest)
    {
        TEST_VERIFY(StringUtils::ToUpperCase(String("abc")) == "ABC");
        TEST_VERIFY(StringUtils::ToUpperCase(String("   abc    ")) == "   ABC    ");
        TEST_VERIFY(StringUtils::ToUpperCase(String("\t\nabc\n\t")) == "\t\nABC\n\t");
        TEST_VERIFY(StringUtils::ToUpperCase(String("    ")) == "    ");
        TEST_VERIFY(StringUtils::ToUpperCase(String("\t \t")) == "\t \t");
    }

    // Utils::ToLowerCase
    DAVA_TEST (ToLowerCase)
    {
        TEST_VERIFY(StringUtils::ToLowerCase(String("ABC")) == "abc");
        TEST_VERIFY(StringUtils::ToLowerCase(String("   ABC    ")) == "   abc    ");
        TEST_VERIFY(StringUtils::ToLowerCase(String("\t\nABC\n\t")) == "\t\nabc\n\t");
        TEST_VERIFY(StringUtils::ToLowerCase(String("    ")) == "    ");
        TEST_VERIFY(StringUtils::ToLowerCase(String("\t \t")) == "\t \t");
    }

    DAVA_TEST (SubstituteParamsTest)
    {
        UnorderedMap<String, String> replacements;
        replacements["a"] = "123";
        replacements["b"] = "Hello, world";
        replacements["param"] = "Param";
        TEST_VERIFY(StringUtils::SubstituteParams(String("bla bla bla str"), replacements) == "bla bla bla str");
        TEST_VERIFY(StringUtils::SubstituteParams(String("bla %(a) %(b) str"), replacements) == "bla 123 Hello, world str");
        TEST_VERIFY(StringUtils::SubstituteParams(String("bla %(b) %(a) str"), replacements) == "bla Hello, world 123 str");
        TEST_VERIFY(StringUtils::SubstituteParams(String("bla %(a) %(c) str"), replacements) == "bla 123 %(c) str");
    }
};
