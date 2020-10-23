#include "Base/Exception.h"
#include "Debug/Backtrace.h"

namespace DAVA
{
Exception::Exception(const String& message, const char* file_, size_t line_)
    : std::runtime_error(message)
    , file(file_)
    , line(line_)
    , callstack(Debug::GetBacktrace())
{
}

Exception::Exception(const char* message, const char* file_, size_t line_)
    : std::runtime_error(message)
    , file(file_)
    , line(line_)
    , callstack(Debug::GetBacktrace())
{
}

} // namespace DAVA
