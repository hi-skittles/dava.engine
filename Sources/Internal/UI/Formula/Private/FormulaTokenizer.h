#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
class FormulaToken
{
public:
    enum Type
    {
        INVALID,
        INT32,
        UINT32,
        INT64,
        UINT64,
        FLOAT,
        BOOLEAN,
        STRING,
        IDENTIFIER,
        COMMA,
        SEMICOLON,
        DOT,
        PLUS,
        MINUS,
        MUL,
        DIV,
        MOD,
        AND,
        OR,
        NOT,
        LE,
        LT,
        GE,
        GT,
        ASSIGN,
        EQ,
        NOT_EQ,
        WHEN,
        ARROW,
        OPEN_BRACKET, // (
        CLOSE_BRACKET, // )
        OPEN_CURLY_BRACKET, // {
        CLOSE_CURLY_BRACKET, // }
        OPEN_SQUARE_BRACKET, // [
        CLOSE_SQUARE_BRACKET, // ]
        END,
    };

    FormulaToken();
    FormulaToken(Type type, int32 lineNumber, int32 positionInLine);
    FormulaToken(Type type, int32 val, int32 lineNumber, int32 positionInLine);
    FormulaToken(Type type, uint32 val, int32 lineNumber, int32 positionInLine);
    FormulaToken(Type type, int64 val, int32 lineNumber, int32 positionInLine);
    FormulaToken(Type type, uint64 val, int32 lineNumber, int32 positionInLine);
    FormulaToken(Type type, float32 val, int32 lineNumber, int32 positionInLine);
    FormulaToken(Type type, bool val, int32 lineNumber, int32 positionInLine);
    FormulaToken(Type type, int32 startPos, int32 len, int32 lineNumber, int32 positionInLine);

    Type GetType() const;
    int32 GetInt32() const;
    uint32 GetUInt32() const;
    int64 GetInt64() const;
    uint64 GetUInt64() const;
    float32 GetFloat() const;
    bool GetBool() const;
    int32 GetStringPos() const;
    int32 GetStringLen() const;

    int32 GetLineNumber() const;
    int32 GetPositionInLine() const;

private:
    Type type;

    union {
        struct
        {
            int32 start;
            int32 len;
        };

        int32 i32Val;
        uint32 ui32Val;
        int64 i64Val;
        uint64 ui64Val;
        float32 fVal;
        bool bVal;
    };

    int32 lineNumber = 0;
    int32 positionInLine = 0;
};

class FormulaTokenizer
{
public:
    FormulaTokenizer(const String& str);
    ~FormulaTokenizer();

    const String& GetString() const;
    String GetTokenStringValue(const FormulaToken& token) const;

    int32 GetLineNumber() const;
    int32 GetPositionInLine() const;

    FormulaToken ReadToken();

private:
    float32 ParseFloat(int32 startPos, int32 endPos, int32 dotPos);

    template <typename T>
    T ParseInt(int32 startPos, int32 endPos);

    void SkipWhitespaces();
    bool IsIdentifierStart(char ch) const;
    bool IsIdentifierPart(char ch) const;
    bool IsDigit(char ch) const;
    void ReadChar();

    String str;
    char ch = '\0';
    int32 currentPosition = -1;
    int32 lineNumber = 0;
    int32 positionInLine = 0;
};
}
