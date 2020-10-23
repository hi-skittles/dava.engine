#pragma once

#include "Base/BaseTypes.h"
#include "FileSystem/XMLParserStatus.h"

namespace DAVA
{
class FilePath;
class XMLParserDelegate;
class XMLParser
{
public:
    /** Parse xml data from specified file and delegate and return error information. */
    static XMLParserStatus ParseFileEx(const FilePath& fileName, XMLParserDelegate* delegate);
    /** Parse xml data from specified buffer and delegate and return error information. */
    static XMLParserStatus ParseBytesEx(const char8* bytes, int32 length, XMLParserDelegate* delegate);
    /** Parse xml data from specified string and delegate and return error information. */
    static XMLParserStatus ParseStringEx(const String& str, XMLParserDelegate* delegate);

    DAVA_DEPRECATED(static bool ParseFile(const FilePath& fileName, XMLParserDelegate* delegate));
    DAVA_DEPRECATED(static bool ParseBytes(const unsigned char* bytes, int length, XMLParserDelegate* delegate));
};
};
