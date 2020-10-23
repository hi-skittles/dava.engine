#include "Scripting/LuaException.h"

namespace DAVA
{
LuaException::LuaException(int32 code, const String& msg, const char* file_, size_t line_)
    : Exception(msg, file_, line_)
    , code(code)
{
}

LuaException::LuaException(int32 code, const char* msg, const char* file_, size_t line_)
    : Exception(msg, file_, line_)
    , code(code)
{
}
}