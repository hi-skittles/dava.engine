#pragma once

#include "Base/BaseTypes.h"
#include "Base/Exception.h"

namespace DAVA
{
/**
Specified Lua errors handling as exception
*/
class LuaException : public Exception
{
public:
    /**
    Create exception with specified error code and message
    */
    LuaException(int32 code, const String& msg, const char* file_, size_t line_);

    /**
    Create exception with specified error code and message
    */
    LuaException(int32 code, const char* msg, const char* file_, size_t line_);

    /**
    Return stored error code
    */
    int32 ErrorCode() const;

private:
    int32 code = -1;
};

inline int32 LuaException::ErrorCode() const
{
    return code;
}
}