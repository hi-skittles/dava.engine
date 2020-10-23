#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
/** Describe information about last error while parsing XML data. */
struct XMLParserStatus
{
    /** Error message. */
    String errorMessage;
    /** Error code. If 0 then success. */
    int32 code = 0;
    /** Line with error. */
    int32 errorLine = 0;
    /** Position in line with error. */
    int32 errorPosition = 0;

    /** Return true if hasn't any error. */
    inline bool Success() const
    {
        return code == 0;
    }
};
}