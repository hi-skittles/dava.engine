#pragma once

#include "Base/BaseTypes.h"
#include "Base/Exception.h"
#include "UI/Formula/Private/FormulaExpression.h"

namespace DAVA
{
/**
 \ingroup formula
 
 Exception class for parsing and execution of formulas with information about
 problem location.
 */
class FormulaException : public Exception
{
public:
    FormulaException(const String& message, int32 lineNumber, int32 positionInLine, const char* file, size_t line);
    FormulaException(const char* message, int32 lineNumber, int32 positionInLine, const char* file, size_t line);
    FormulaException(const String& message, const FormulaExpression* exp, const char* file, size_t line);
    FormulaException(const char* message, const FormulaExpression* exp, const char* file, size_t line);
    ~FormulaException();

    int32 GetLineNumber() const;
    int32 GetPositionInLine() const;
    String GetErrorMessage() const;
    String GetFormattedMessage() const;

private:
    int32 lineNumber = -1;
    int32 positionInLine = -1;
};
}
