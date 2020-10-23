#pragma once

#include <stdexcept>
#include "Base/String.h"
#include "Base/Vector.h"

namespace DAVA
{
struct Exception : std::runtime_error
{
    Exception(const String& message, const char* file, size_t line);
    Exception(const char*, const char* file, size_t line);

    String file;
    size_t line;
    Vector<void*> callstack;
};
} // namespace DAVA

#define DAVA_THROW(e, ...) throw e(__VA_ARGS__, __FILE__, __LINE__)
