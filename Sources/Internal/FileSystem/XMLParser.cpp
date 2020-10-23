#include "FileSystem/XMLParser.h"
#include "FileSystem/File.h"
#include "FileSystem/XMLParserDelegate.h"
#include "Logger/Logger.h"

#include <libxml/parser.h>
#include <libxml/xmlstring.h>

namespace DAVA
{
namespace XMLParserDetails
{
struct UserData
{
    XMLParserDelegate* delegate = nullptr;
};

static void Characters(void* user_data, const xmlChar* ch, int len)
{
    UserData* data = reinterpret_cast<UserData*>(user_data);
    if (data->delegate)
    {
        String s(reinterpret_cast<const char*>(ch), len);
        data->delegate->OnFoundCharacters(s);
    }
}

static void StartElement(void* user_data, const xmlChar* name, const xmlChar** attrs)
{
    UserData* data = reinterpret_cast<UserData*>(user_data);
    if (data->delegate)
    {
        Map<String, String> attributes;
        if (attrs)
        {
            int32 i = 0;
            while (attrs[i])
            {
                const char* str = (attrs[i + 1]) ? reinterpret_cast<const char*>(attrs[i + 1]) : "";
                attributes[reinterpret_cast<const char*>(attrs[i])] = str;
                i += 2;
            }
        }
        data->delegate->OnElementStarted(reinterpret_cast<const char*>(name), "", "", attributes);
    }
}

static void EndElement(void* user_data, const xmlChar* name)
{
    UserData* data = reinterpret_cast<UserData*>(user_data);
    if (data->delegate)
    {
        data->delegate->OnElementEnded(reinterpret_cast<const char*>(name), "", "");
    }
}
}

XMLParserStatus XMLParser::ParseFileEx(const FilePath& fileName, XMLParserDelegate* delegateptr)
{
    XMLParserStatus status;
    File* xmlFile = File::Create(fileName, File::OPEN | File::READ);
    if (xmlFile)
    {
        uint64 dataSize = xmlFile->GetSize();
        char8* data = new char8[static_cast<size_t>(dataSize)];
        if (data)
        {
            uint32 readBytes = xmlFile->Read(data, static_cast<uint32>(dataSize));
            if (readBytes == dataSize)
            {
                status = XMLParser::ParseBytesEx(data, static_cast<uint32>(dataSize), delegateptr);
            }
            else
            {
                status.code = xmlParserErrors::XML_ERR_INTERNAL_ERROR;
                status.errorMessage = "[XMLParser::ParseFile] readBytes != dataSize";
            }
            SafeDeleteArray(data);
        }
        else
        {
            status.code = xmlParserErrors::XML_ERR_INTERNAL_ERROR;
            status.errorMessage = "[XMLParser::ParseFile] Can't allocate data";
        }
        SafeRelease(xmlFile);
    }
    else
    {
        status.code = xmlParserErrors::XML_ERR_INTERNAL_ERROR;
        status.errorMessage = Format("[XMLParser::ParseFile] Can't open file %s for read", fileName.GetStringValue().c_str());
    }
    return status;
}

XMLParserStatus XMLParser::ParseBytesEx(const char8* bytes, int32 length, XMLParserDelegate* delegateptr)
{
    XMLParserStatus status;

    xmlSAXHandler saxHandler = { 0 };
    saxHandler.startElement = XMLParserDetails::StartElement;
    saxHandler.endElement = XMLParserDetails::EndElement;
    saxHandler.characters = XMLParserDetails::Characters;

    XMLParserDetails::UserData data;
    data.delegate = delegateptr;

    // Always use pointer to valid user data structure because if we send nullptr
    // as user_data to xmlSAXUserParseMemory then pointer to xmlParserCtxtPtr
    // will come to saxHandler's functions instead our nullptr.
    int32 retCode = xmlSAXUserParseMemory(&saxHandler, reinterpret_cast<void*>(&data), reinterpret_cast<const char*>(bytes), static_cast<int>(length));

    if (retCode != xmlParserErrors::XML_ERR_OK)
    {
        xmlErrorPtr err = xmlGetLastError();
        if (err != nullptr)
        {
            status.code = static_cast<int32>(err->code);
            status.errorMessage = String(err->message);
            status.errorLine = static_cast<int32>(err->line);
            status.errorPosition = static_cast<int32>(err->int2);
        }
        else
        {
            status.code = retCode;
            status.errorMessage = "[XMLParser::ParseBytesEx] Unknown internal error";
        }
    }

    return status;
}

XMLParserStatus XMLParser::ParseStringEx(const String& str, XMLParserDelegate* delegate)
{
    return ParseBytesEx(str.c_str(), static_cast<int32>(str.length()), delegate);
}

bool XMLParser::ParseFile(const FilePath& fileName, XMLParserDelegate* delegateptr)
{
    XMLParserStatus status = ParseFileEx(fileName, delegateptr);
    if (!status.Success())
    {
        Logger::Error(status.errorMessage.c_str());
        return false;
    }
    return true;
}

bool XMLParser::ParseBytes(const unsigned char* bytes, int length, XMLParserDelegate* delegateptr)
{
    XMLParserStatus status = ParseBytesEx(reinterpret_cast<const char8*>(bytes), static_cast<int32>(length), delegateptr);
    if (!status.Success())
    {
        Logger::Error(status.errorMessage.c_str());
        return false;
    }
    return true;
}
};
