#include "UI/Formula/Private/FormulaException.h"
#include "Utils/StringFormat.h"

namespace DAVA
{
FormulaException::FormulaException(const String& message, int32 lineNumber_, int32 positionInLine_, const char* file, size_t line)
    : Exception(message, file, line)
    , lineNumber(lineNumber_)
    , positionInLine(positionInLine_)
{
}

FormulaException::FormulaException(const char* message, int32 lineNumber_, int32 positionInLine_, const char* file, size_t line)
    : Exception(message, file, line)
    , lineNumber(lineNumber_)
    , positionInLine(positionInLine_)
{
}

FormulaException::FormulaException(const String& message, const FormulaExpression* exp, const char* file, size_t line)
    : Exception(message, file, line)
    , lineNumber(exp->GetLineNumber())
    , positionInLine(exp->GetPositionInLine())
{
}

FormulaException::FormulaException(const char* message, const FormulaExpression* exp, const char* file, size_t line)
    : Exception(message, file, line)
    , lineNumber(exp->GetLineNumber())
    , positionInLine(exp->GetPositionInLine())
{
}

FormulaException::~FormulaException()
{
}

int32 FormulaException::GetLineNumber() const
{
    return lineNumber;
}

int32 FormulaException::GetPositionInLine() const
{
    return positionInLine;
}

String FormulaException::GetErrorMessage() const
{
    return what();
}

String FormulaException::GetFormattedMessage() const
{
    return Format("[%d, %d] %s", lineNumber, positionInLine, what());
}
}
