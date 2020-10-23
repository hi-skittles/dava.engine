#ifndef __DAVAENGINE_UTILS_H__
#define __DAVAENGINE_UTILS_H__

/**
	\defgroup utils Utilities
 */

#include "Base/BaseTypes.h"
#include "FileSystem/FilePath.h"
#include "Render/RenderBase.h"
#include <sstream>
#include "Utils/UTF8Utils.h"

#ifdef __DAVAENGINE_WIN_UAP__
#include <ppltasks.h>
#endif

namespace DAVA
{
int read_handler(void* ext, unsigned char* buffer, size_t size, size_t* length);

WideString WcharToWString(const wchar_t* s);

bool IsDrawThread();

WideString GetDeviceName();

void DisableSleepTimer();
void EnableSleepTimer();

void Split(const String& inputString, const String& delims, Vector<String>& tokens, bool skipDuplicated = false, bool addEmptyTokens = false, bool integralDelim = false);
void Merge(const Vector<String>& tokens, const char delim, String& outString);
void ReplaceBundleName(const String& newBundlePath);

template <class T>
T ParseStringTo(const String& str);

template <class T>
bool ParseFromString(const String& str, T& res);

template <class T>
void Swap(T& v1, T& v2);

/**
 \brief Function to compare strings case-insensitive
 \param[in] ext1 - first string
 \param[in] ext2 - second string
 \param[out] result of comparison
 */
int32 CompareCaseInsensitive(const String& str1, const String& str2);

//implementation

inline void StringReplace(String& repString, const String& needle, const String& s)
{
    String::size_type lastpos = 0, thispos;
    while ((thispos = repString.find(needle, lastpos)) != String::npos)
    {
        repString.replace(thispos, needle.length(), s);
        lastpos = thispos + s.length();
    }
}

#if defined(__DAVAENGINE_WIN_UAP__)
inline Platform::String ^ StringToRTString(const String& s)
{
    return ref new Platform::String(UTF8Utils::EncodeToWideString(s).c_str());
}

inline String RTStringToString(Platform::String ^ s)
{
    return UTF8Utils::EncodeToUTF8(s->Data());
}
#endif

template <class T>
bool FindAndRemoveExchangingWithLast(Vector<T>& array, const T& object)
{
    size_t size = array.size();
    for (size_t k = 0; k < size; ++k)
    {
        if (array[k] == object)
        {
            array[k] = array[size - 1];
            array.pop_back();
            return true;
        }
    }
    return false;
}

template <class T>
void RemoveExchangingWithLast(Vector<T>& array, size_t index)
{
    array[index] = array[array.size() - 1];
    array.pop_back();
}

template <class T>
T ParseStringTo(const String& str)
{
    T result;
    std::stringstream stream(str);
    stream >> result;
    return result;
}

template <class T>
bool ParseFromString(const String& str, T& result)
{
    std::stringstream stream(str);
    stream >> result;
    return (stream.eof() == true && stream.fail() == false);
}

template <class T>
void Swap(T& v1, T& v2)
{
    T temp = v1;
    v1 = v2;
    v2 = temp;
}

template <class T, std::size_t size>
class CircularArray
{
public:
    T& Next()
    {
        T& ret = elements[currentIndex];

        if ((++currentIndex) == elements.size())
            currentIndex = 0;

        return ret;
    }

    std::array<T, size> elements;

protected:
    std::size_t currentIndex = 0;
};

// Open the URL in external browser.
void OpenURL(const String& url);

String GenerateGUID();

#ifdef __DAVAENGINE_WIN_UAP__
template <typename T>
T WaitAsync(Windows::Foundation::IAsyncOperation<T> ^ async_operation)
{
    return concurrency::create_task(async_operation).get();
}
#endif
#ifdef __DAVAENGINE_WIN32__
Vector<String> GetCommandLineArgs();
#endif // __DAVAENGINE_WIN32__
};

#endif // __DAVAENGINE_UTILS_H__
