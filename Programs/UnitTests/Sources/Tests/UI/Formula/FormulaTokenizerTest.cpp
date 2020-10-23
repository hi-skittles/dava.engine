#include "DAVAEngine.h"

#include "UI/Formula/Private/FormulaParser.h"
#include "UI/Formula/Private/FormulaException.h"
#include "UI/Formula/Private/FormulaTokenizer.h"

#include "UnitTests/UnitTests.h"

using namespace DAVA;

DAVA_TESTCLASS (FormulaTokenizerTest)
{
    void SetUp(const String& testName) override
    {
    }

    void TearDown(const String& testName) override
    {
    }

    // FormulaTokenizer::ReadToken
    DAVA_TEST (ReadStringsAndIdentifiers)
    {
        FormulaTokenizer tokenizer("\"Hello\" foo A123 a123()");

        FormulaToken token;

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::STRING);
        TEST_VERIFY(tokenizer.GetTokenStringValue(token) == "Hello");

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::IDENTIFIER);
        TEST_VERIFY(tokenizer.GetTokenStringValue(token) == "foo");

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::IDENTIFIER);
        TEST_VERIFY(tokenizer.GetTokenStringValue(token) == "A123");

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::IDENTIFIER);
        TEST_VERIFY(tokenizer.GetTokenStringValue(token) == "a123");

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::OPEN_BRACKET);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::CLOSE_BRACKET);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::END);
    }

    // FormulaTokenizer::ReadToken
    DAVA_TEST (ReadFloats)
    {
        FormulaTokenizer tokenizer("5.5 0.0 1.0 0.00001 11111.0");

        FormulaToken token;

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::FLOAT);
        TEST_VERIFY(FLOAT_EQUAL(token.GetFloat(), 5.5f));

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::FLOAT);
        TEST_VERIFY(FLOAT_EQUAL(token.GetFloat(), 0.0f));

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::FLOAT);
        TEST_VERIFY(FLOAT_EQUAL(token.GetFloat(), 1.0f));

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::FLOAT);
        TEST_VERIFY(FLOAT_EQUAL(token.GetFloat(), 0.00001f));

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::FLOAT);
        TEST_VERIFY(FLOAT_EQUAL(token.GetFloat(), 11111.0f));

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::END);
    }

    // FormulaTokenizer::ReadToken
    DAVA_TEST (ReadBooleans)
    {
        FormulaTokenizer tokenizer("true false");

        FormulaToken token;

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::BOOLEAN);
        TEST_VERIFY(token.GetBool() == true);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::BOOLEAN);
        TEST_VERIFY(token.GetBool() == false);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::END);
    }

    // FormulaTokenizer::ReadToken
    DAVA_TEST (ReadInts)
    {
        FormulaTokenizer tokenizer("0 2147483647 0U 4294967295U 0L 9223372036854775807L 0LU 18446744073709551615UL");

        FormulaToken token;

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::INT32);
        TEST_VERIFY(token.GetInt32() == 0);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::INT32);
        TEST_VERIFY(token.GetInt32() == 2147483647);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::UINT32);
        TEST_VERIFY(token.GetUInt32() == 0U);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::UINT32);
        TEST_VERIFY(token.GetUInt32() == 4294967295U);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::INT64);
        TEST_VERIFY(token.GetInt64() == 0L);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::INT64);
        TEST_VERIFY(token.GetInt64() == 9223372036854775807L);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::UINT64);
        TEST_VERIFY(token.GetUInt64() == 0UL);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::UINT64);
        TEST_VERIFY(token.GetUInt64() == 18446744073709551615UL);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::END);
    }

    // FormulaTokenizer::ReadToken
    DAVA_TEST (ReadOperatorTokens)
    {
        FormulaTokenizer tokenizer(", . + - * / % and or not <= < >= > = == ; != when ->");

        FormulaToken token;

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::COMMA);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::DOT);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::PLUS);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::MINUS);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::MUL);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::DIV);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::MOD);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::AND);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::OR);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::NOT);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::LE);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::LT);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::GE);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::GT);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::ASSIGN);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::EQ);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::SEMICOLON);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::NOT_EQ);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::WHEN);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::ARROW);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::END);
    }

    // FormulaTokenizer::ReadToken
    DAVA_TEST (ReadBracketTokens)
    {
        FormulaTokenizer tokenizer("() {} []");

        FormulaToken token;

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::OPEN_BRACKET);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::CLOSE_BRACKET);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::OPEN_CURLY_BRACKET);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::CLOSE_CURLY_BRACKET);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::OPEN_SQUARE_BRACKET);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::CLOSE_SQUARE_BRACKET);

        token = tokenizer.ReadToken();
        TEST_VERIFY(token.GetType() == FormulaToken::END);
    }

    // FormulaTokenizer::ReadToken
    DAVA_TEST (ReadTokensWithErrors)
    {
        bool wasException = false;
        try
        {
            FormulaTokenizer("&").ReadToken();
        }
        catch (const FormulaException& /*error*/)
        {
            wasException = true;
        }
        TEST_VERIFY(wasException == true);

        wasException = false;
        try
        {
            FormulaTokenizer("|").ReadToken();
        }
        catch (const FormulaException& /*error*/)
        {
            wasException = true;
        }
        TEST_VERIFY(wasException == true);

        wasException = false;
        try
        {
            FormulaTokenizer("@").ReadToken();
        }
        catch (const FormulaException& /*error*/)
        {
            wasException = true;
        }
        TEST_VERIFY(wasException == true);

        wasException = false;
        try
        {
            FormulaTokenizer("@").ReadToken();
        }
        catch (const FormulaException& /*error*/)
        {
            wasException = true;
        }
        TEST_VERIFY(wasException == true);

        wasException = false;
        try
        {
            FormulaTokenizer("\"sadfds ").ReadToken();
        }
        catch (const FormulaException& /*error*/)
        {
            wasException = true;
        }
        TEST_VERIFY(wasException == true);
    }
};
