#pragma once
#include "Base/BaseTypes.h"

namespace DAVA
{
namespace Debug
{
DAVA_NOINLINE Vector<void*> GetBacktrace(size_t depth = -1);
DAVA_NOINLINE size_t GetBacktrace(void** frames, size_t framesSize);

String GetBacktraceString(size_t depth = -1);
String GetBacktraceString(const Vector<void*>& backtrace);
String GetBacktraceString(const Vector<void*>& backtrace, size_t framesSize);
String GetBacktraceString(void* const* frames, size_t framesSize);

String GetFrameSymbol(void* frame, bool demangle = true);
String DemangleFrameSymbol(const char8* symbol);
String DemangleFrameSymbol(const String& symbol);

} // namespace Debug
} // namespace DAVA
