#pragma once

#include "ExpressionEvaluator.h"

namespace DAVA
{
class PreProc
{
public:
    struct FileCallback
    {
        virtual ~FileCallback() = default;
        virtual bool Open(const char* /*file_name*/) = 0;
        virtual void Close() = 0;
        virtual uint32 Size() const = 0;
        virtual uint32 Read(uint32 /*max_sz*/, void* /*dst*/) = 0;
    };
    using TextBuffer = std::vector<char>;

public:
    PreProc(FileCallback* fc = nullptr);
    ~PreProc();

    bool ProcessFile(const char* file_name, TextBuffer* output);
    bool Process(const char* src_text, TextBuffer* output);
    void Clear();

    bool AddDefine(const char* name, const char* value);

private:
    struct Line
    {
        uint32 line_n;
        const char* text;
        Line(const char* t, uint32 n)
            : text(t)
            , line_n(n)
        {
        }
    };
    using LineVector = std::vector<Line>;

    void Reset();
    char* AllocBuffer(uint32 sz);
    bool ProcessInplaceInternal(char* src_text, TextBuffer* output);
    bool ProcessBuffer(char* text, LineVector& line);
    bool ProcessInclude(const char* file_name, LineVector& line);
    bool ProcessDefine(const char* name, const char* val);
    void Undefine(const char* name);
    void GenerateOutput(TextBuffer* output, LineVector& line);

    char* GetExpression(char* txt, char** end) const;
    char* GetIdentifier(char* txt, char** end) const;
    int32 GetNameAndValue(char* txt, char** name, char** value, char** end) const;
    void ReportExprEvalError(uint32 line_n);
    char* ExpandMacroInLine(char* txt);

    char* GetNextToken(char* txt, ptrdiff_t txtSize, ptrdiff_t& tokenSize);
    const char* GetNextToken(const char* txt, ptrdiff_t txtSize, ptrdiff_t& tokenSize);

public:
    enum : uint32
    {
        MaxMacroValueLength = 128,
        MaxLocalStringLength = 2048,
    };

    struct MacroStringBuffer
    {
        const char* value = nullptr;
        uint32 length = 0;

        MacroStringBuffer() = default;

        MacroStringBuffer(const char* nm, uint32 sz)
            : value(nm)
            , length(sz)
        {
        }

        bool operator==(const MacroStringBuffer& r) const
        {
            bool equals = false;
            if (length == r.length)
            {
                equals = (strncmp(value, r.value, length) == 0);
            }
            return equals;
        }
    };

    struct MacroStringBufferHash
    {
        uint64 operator()(const MacroStringBuffer& m) const
        {
            return DAVA::HashValue_N(m.value, m.length);
        }
    };

    using MacroMap = UnorderedMap<MacroStringBuffer, MacroStringBuffer, MacroStringBufferHash>;

private:
    enum : uint32
    {
        InvalidValue = static_cast<uint32>(-1)
    };

    enum : char
    {
        Tab = '\t',
        Zero = '\0',
        Space = ' ',
        NewLine = '\n',
        DoubleQuotes = '\"',
        CarriageReturn = '\r',
    };

    MacroMap macro;
    Vector<char*> buffer;
    ExpressionEvaluator evaluator;
    FileCallback* fileCB = nullptr;
    const char* curFileName = "<buffer>";
};
}