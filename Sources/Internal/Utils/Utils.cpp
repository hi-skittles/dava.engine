#include "Utils/UTF8Utils.h"
#include "Utils/StringFormat.h"
#include "FileSystem/YamlParser.h"

#ifdef __DAVAENGINE_WIN32__
#include <shellapi.h>
#endif // __DAVAENGINE_WIN32__

namespace DAVA
{
WideString WcharToWString(const wchar_t* s)
{
    return WideString(s);
}

void Split(const String& inputString, const String& delims, Vector<String>& tokens, bool skipDuplicated /* = false*/, bool addEmptyTokens /* = false*/, bool integralDelim /* = false*/)
{
    std::string::size_type pos, lastPos = 0;
    bool needAddToken = true;
    bool exit = false;
    String token = "";
    size_t delimLen = (integralDelim ? delims.size() : 1);

    while (true)
    {
        needAddToken = false;
        if (integralDelim)
        {
            pos = inputString.find(delims, lastPos);
        }
        else
        {
            pos = inputString.find_first_of(delims, lastPos);
        }

        if (pos == std::string::npos)
        {
            pos = inputString.length();
            exit = true;
        }
        if (pos != lastPos || addEmptyTokens)
            needAddToken = true;

        token = String(inputString.data() + lastPos, pos - lastPos);
        if (skipDuplicated && needAddToken)
        {
            for (uint32 i = 0; i < tokens.size(); ++i)
            {
                if (token.compare(tokens[i]) == 0)
                {
                    needAddToken = false;
                    break;
                }
            }
        }
        if (needAddToken)
            tokens.push_back(token);
        if (exit)
            break;

        lastPos = pos + delimLen;
    }
}

void Merge(const Vector<String>& tokens, const char delim, String& outString)
{
    outString.clear();

    size_t tokensSize = tokens.size();
    if (tokensSize > 0)
    {
        outString.append(tokens[0]);
        if (tokensSize > 1)
        {
            for (size_t i = 1; i < tokensSize; ++i)
            {
                outString += delim;
                outString += tokens[i];
            }
        }
    }
}

/* Set a generic reader. */
int read_handler(void* ext, unsigned char* buffer, size_t size, size_t* length)
{
    YamlParser::YamlDataHolder* holder = static_cast<YamlParser::YamlDataHolder*>(ext);
    size_t sizeToWrite = Min(size, static_cast<size_t>(holder->fileSize - holder->dataOffset));

    memcpy(buffer, holder->data + holder->dataOffset, sizeToWrite);

    *length = sizeToWrite;

    holder->dataOffset += static_cast<uint32>(sizeToWrite);

    return 1;
}

int32 CompareCaseInsensitive(const String& str1, const String& str2)
{
    String newStr1 = "";
    newStr1.resize(str1.length());
    std::transform(str1.begin(), str1.end(), newStr1.begin(), ::tolower);

    String newStr2 = "";
    newStr2.resize(str2.length());
    std::transform(str2.begin(), str2.end(), newStr2.begin(), ::tolower);

    if (newStr1 == newStr2)
    {
        return 0;
    }
    else if (newStr1 < newStr2)
    {
        return -1;
    }

    return 1;
}

#if !defined(__DAVAENGINE_IPHONE__) && !defined(__DAVAENGINE_ANDROID__)
void DisableSleepTimer()
{
}

void EnableSleepTimer()
{
}

#endif

#ifdef __DAVAENGINE_WIN32__
Vector<String> GetCommandLineArgs()
{
    int argc = 0;
    Vector<String> args;
    LPWSTR* szArglist = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

    if (argc > 0 && NULL != szArglist)
    {
        args.reserve(argc);
        for (int i = 0; i < argc; ++i)
        {
            args.emplace_back(UTF8Utils::EncodeToUTF8(szArglist[i]));
        }
    }
    ::LocalFree(szArglist);
    return args;
}
#endif // __DAVAENGINE_WIN32__

}; // end of namespace DAVA
