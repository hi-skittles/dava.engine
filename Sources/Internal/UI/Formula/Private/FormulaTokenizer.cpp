#include "UI/Formula/Private/FormulaTokenizer.h"

#include "UI/Formula/Private/FormulaException.h"

#include "Debug/DVAssert.h"
#include "Utils/StringFormat.h"

namespace DAVA
{
FormulaToken::FormulaToken()
    : type(INVALID)
{
}

FormulaToken::FormulaToken(Type type_, int32 lineNumber_, int32 positionInLine_)
    : type(type_)
    , lineNumber(lineNumber_)
    , positionInLine(positionInLine_)
{
    DVASSERT(type >= COMMA && type <= END);
}

FormulaToken::FormulaToken(Type type_, int32 val_, int32 lineNumber_, int32 positionInLine_)
    : type(type_)
    , i32Val(val_)
    , lineNumber(lineNumber_)
    , positionInLine(positionInLine_)
{
    DVASSERT(type == INT32);
}

FormulaToken::FormulaToken(Type type_, uint32 val_, int32 lineNumber_, int32 positionInLine_)
    : type(type_)
    , ui32Val(val_)
    , lineNumber(lineNumber_)
    , positionInLine(positionInLine_)
{
    DVASSERT(type == UINT32);
}

FormulaToken::FormulaToken(Type type_, int64 val_, int32 lineNumber_, int32 positionInLine_)
    : type(type_)
    , i64Val(val_)
    , lineNumber(lineNumber_)
    , positionInLine(positionInLine_)
{
    DVASSERT(type == INT64);
}

FormulaToken::FormulaToken(Type type_, uint64 val_, int32 lineNumber_, int32 positionInLine_)
    : type(type_)
    , ui64Val(val_)
    , lineNumber(lineNumber_)
    , positionInLine(positionInLine_)
{
    DVASSERT(type == UINT64);
}

FormulaToken::FormulaToken(Type type_, float32 val_, int32 lineNumber_, int32 positionInLine_)
    : type(type_)
    , fVal(val_)
    , lineNumber(lineNumber_)
    , positionInLine(positionInLine_)
{
    DVASSERT(type == FLOAT);
}

FormulaToken::FormulaToken(Type type_, bool val_, int32 lineNumber_, int32 positionInLine_)
    : type(type_)
    , bVal(val_)
    , lineNumber(lineNumber_)
    , positionInLine(positionInLine_)
{
    DVASSERT(type == BOOLEAN);
}

FormulaToken::FormulaToken(Type type_, int32 startPos_, int32 len_, int32 lineNumber_, int32 positionInLine_)
    : type(type_)
    , start(startPos_)
    , len(len_)
    , lineNumber(lineNumber_)
    , positionInLine(positionInLine_)
{
    DVASSERT(type == IDENTIFIER || type == STRING);
}

FormulaToken::Type FormulaToken::GetType() const
{
    return type;
}

int32 FormulaToken::GetInt32() const
{
    DVASSERT(type == INT32);
    return i32Val;
}

uint32 FormulaToken::GetUInt32() const
{
    DVASSERT(type == UINT32);
    return ui32Val;
}

int64 FormulaToken::GetInt64() const
{
    DVASSERT(type == INT64);
    return i64Val;
}

uint64 FormulaToken::GetUInt64() const
{
    DVASSERT(type == UINT64);
    return ui64Val;
}

float32 FormulaToken::GetFloat() const
{
    DVASSERT(type == FLOAT);
    return fVal;
}

bool FormulaToken::GetBool() const
{
    DVASSERT(type == BOOLEAN);
    return bVal;
}

int32 FormulaToken::GetStringPos() const
{
    DVASSERT(type == IDENTIFIER || type == STRING);
    return start;
}

int32 FormulaToken::GetStringLen() const
{
    DVASSERT(type == IDENTIFIER || type == STRING);
    return len;
}

int32 FormulaToken::GetLineNumber() const
{
    return lineNumber;
}

int32 FormulaToken::GetPositionInLine() const
{
    return positionInLine;
}

FormulaTokenizer::FormulaTokenizer(const String& str_)
    : str(str_)
{
}

FormulaTokenizer::~FormulaTokenizer()
{
}

const String& FormulaTokenizer::GetString() const
{
    return str;
}

String FormulaTokenizer::GetTokenStringValue(const FormulaToken& token) const
{
    return str.substr(token.GetStringPos(), token.GetStringLen());
}

int32 FormulaTokenizer::GetLineNumber() const
{
    return lineNumber;
}

int32 FormulaTokenizer::GetPositionInLine() const
{
    return positionInLine;
}

FormulaToken FormulaTokenizer::ReadToken()
{
    if (currentPosition == -1)
    {
        lineNumber = 1;
        ReadChar();
    }
    SkipWhitespaces();

    int32 line = lineNumber;
    int32 column = positionInLine;
    switch (ch)
    {
    case '\0':
        ReadChar();
        return FormulaToken(FormulaToken::END, line, column);

    case ',':
        ReadChar();
        return FormulaToken(FormulaToken::COMMA, line, column);

    case ';':
        ReadChar();
        return FormulaToken(FormulaToken::SEMICOLON, line, column);

    case '.':
        ReadChar();
        return FormulaToken(FormulaToken::DOT, line, column);

    case '<':
        ReadChar();
        if (ch == '=')
        {
            ReadChar();
            return FormulaToken(FormulaToken::LE, line, column);
        }
        return FormulaToken(FormulaToken::LT, line, column);

    case '>':
        ReadChar();
        if (ch == '=')
        {
            ReadChar();
            return FormulaToken(FormulaToken::GE, line, column);
        }
        return FormulaToken(FormulaToken::GT, line, column);

    case '+':
        ReadChar();
        return FormulaToken(FormulaToken::PLUS, line, column);

    case '-':
        ReadChar();
        if (ch == '>')
        {
            ReadChar();
            return FormulaToken(FormulaToken::ARROW, line, column);
        }
        return FormulaToken(FormulaToken::MINUS, line, column);

    case '*':
        ReadChar();
        return FormulaToken(FormulaToken::MUL, line, column);

    case '/':
        ReadChar();
        return FormulaToken(FormulaToken::DIV, line, column);

    case '%':
        ReadChar();
        return FormulaToken(FormulaToken::MOD, line, column);

    case '=':
        ReadChar();
        if (ch == '=')
        {
            ReadChar();
            return FormulaToken(FormulaToken::EQ, line, column);
        }
        return FormulaToken(FormulaToken::ASSIGN, line, column);

    case '!':
        ReadChar();
        if (ch == '=')
        {
            ReadChar();
            return FormulaToken(FormulaToken::NOT_EQ, line, column);
        }
        DAVA_THROW(FormulaException, "Can't resolve symbol '!'", line, column);

    case '(':
        ReadChar();
        return FormulaToken(FormulaToken::OPEN_BRACKET, line, column);

    case ')':
        ReadChar();
        return FormulaToken(FormulaToken::CLOSE_BRACKET, line, column);

    case '{':
        ReadChar();
        return FormulaToken(FormulaToken::OPEN_CURLY_BRACKET, line, column);

    case '}':
        ReadChar();
        return FormulaToken(FormulaToken::CLOSE_CURLY_BRACKET, line, column);

    case '[':
        ReadChar();
        return FormulaToken(FormulaToken::OPEN_SQUARE_BRACKET, line, column);

    case ']':
        ReadChar();
        return FormulaToken(FormulaToken::CLOSE_SQUARE_BRACKET, line, column);

    case '"':
    {
        ReadChar();

        int32 p = currentPosition;
        while (ch != '\0' && ch != '"')
        {
            ReadChar();
        }

        if (ch == '\0')
        {
            DAVA_THROW(FormulaException, "Illegal line end in string literal", lineNumber, positionInLine);
        }
        ReadChar();

        return FormulaToken(FormulaToken::STRING, p, currentPosition - p - 1, line, column);
    }

    default:
        // do nothing
        break;
    }

    if (IsIdentifierStart(ch))
    {
        int32 p = currentPosition;
        while (IsIdentifierPart(ch))
        {
            ReadChar();
        }

        int32 len = currentPosition - p;
        String id = str.substr(p, len);
        if (id == "true")
        {
            return FormulaToken(FormulaToken::BOOLEAN, true, line, column);
        }
        else if (id == "false")
        {
            return FormulaToken(FormulaToken::BOOLEAN, false, line, column);
        }
        else if (id == "when")
        {
            return FormulaToken(FormulaToken::WHEN, line, column);
        }
        else if (id == "and")
        {
            return FormulaToken(FormulaToken::AND, line, column);
        }
        else if (id == "not")
        {
            return FormulaToken(FormulaToken::NOT, line, column);
        }
        else if (id == "or")
        {
            return FormulaToken(FormulaToken::OR, line, column);
        }

        return FormulaToken(FormulaToken::IDENTIFIER, p, len, line, column);
    }

    if (IsDigit(ch))
    {
        int32 first = currentPosition;
        int32 last = currentPosition;

        while (IsDigit(ch))
        {
            ReadChar();
        }
        last = currentPosition - 1;

        if (ch == '.')
        {
            int32 dotPos = currentPosition;
            ReadChar();

            while (IsDigit(ch))
            {
                ReadChar();
            }

            return FormulaToken(FormulaToken::FLOAT, ParseFloat(first, currentPosition - 1, dotPos), lineNumber, first);
        }
        else if (ch == 'L' || ch == 'l')
        {
            ReadChar();
            if (ch == 'U' || ch == 'u')
            {
                ReadChar();

                return FormulaToken(FormulaToken::UINT64, ParseInt<uint64>(first, last), lineNumber, first);
            }
            else
            {
                return FormulaToken(FormulaToken::INT64, ParseInt<int64>(first, last), lineNumber, first);
            }
        }
        else if (ch == 'U' || ch == 'u')
        {
            ReadChar();
            if (ch == 'L' || ch == 'l')
            {
                ReadChar();

                return FormulaToken(FormulaToken::UINT64, ParseInt<uint64>(first, last), lineNumber, first);
            }
            else
            {
                return FormulaToken(FormulaToken::UINT32, ParseInt<uint32>(first, last), lineNumber, first);
            }
        }
        return FormulaToken(FormulaToken::INT32, ParseInt<int32>(first, last), lineNumber, first);
    }
    DAVA_THROW(FormulaException, "Can't resolve symbol", lineNumber, positionInLine);
}

float32 FormulaTokenizer::ParseFloat(int32 startPos, int32 endPos, int32 dotPos)
{
    float32 val = 0;

    for (int32 index = startPos; index < dotPos; index++)
    {
        val = val * 10.0f + (str[index] - '0');
    }

    float32 fractionalPart = 0.0f;
    for (int32 index = endPos; index > dotPos; index--)
    {
        fractionalPart += (str[index] - '0');
        fractionalPart *= 0.1f;
    }

    return val + fractionalPart;
}

template <typename T>
T FormulaTokenizer::ParseInt(int32 startPos, int32 endPos)
{
    T val = 0;

    for (int32 index = startPos; index <= endPos; index++)
    {
        val = val * 10 + (str[index] - '0');
    }

    return val;
}

void FormulaTokenizer::SkipWhitespaces()
{
    int32 prevCh = 0;
    while (ch == ' ' || ch == '\t' || ch == 10 || ch == 13)
    {
        if (ch == 10 || ch == 13)
        {
            if (prevCh != 13 || ch != 10)
            {
                lineNumber++;
            }
            positionInLine = 0;
        }
        prevCh = ch;
        ReadChar();
    }
}

bool FormulaTokenizer::IsIdentifierStart(char ch) const
{
    return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == '_';
}

bool FormulaTokenizer::IsIdentifierPart(char ch) const
{
    return IsIdentifierStart(ch) || IsDigit(ch);
}

bool FormulaTokenizer::IsDigit(char ch) const
{
    return '0' <= ch && ch <= '9';
}

void FormulaTokenizer::ReadChar()
{
    currentPosition++;
    if (currentPosition < static_cast<int32>(str.size()))
    {
        ch = str.at(currentPosition);
        positionInLine++;
    }
    else
    {
        ch = '\0';
        currentPosition = static_cast<int32>(str.size());
    }
}
}
