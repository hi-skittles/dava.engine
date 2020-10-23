#include "ReflectionDeclaration/Private/AnyCasts.h"

#include "Base/Any.h"
#include "Base/BaseTypes.h"
#include "Base/FastName.h"

#include "Math/Matrix2.h"
#include "Math/Matrix3.h"
#include "Math/Matrix4.h"

#include "FileSystem/FilePath.h"

#include "Utils/StringFormat.h"
#include "Utils/UTF8Utils.h"
#include "Render/RenderBase.h"

namespace DAVA
{
const char* StringToCharPointer(const Any& value)
{
    return value.Get<String>().c_str();
}

FastName StringToFastName(const Any& value)
{
    return FastName(value.Get<String>().c_str());
}

String CharPointerToString(const Any& value)
{
    return String(value.Get<const char*>());
}

FastName CharPointerToFastName(const Any& value)
{
    return FastName(value.Get<const char*>());
}

WideString StringToWideString(const Any& value)
{
    return UTF8Utils::EncodeToWideString(value.Get<String>());
}

String WideStringToString(const Any& value)
{
    return UTF8Utils::EncodeToUTF8(value.Cast<WideString>());
}

const char* FastNameToCharPointer(const Any& value)
{
    const FastName& v = value.Get<FastName>();
    if (v.IsValid() == false)
    {
        return nullptr;
    }
    return v.c_str();
}

String FastNameToString(const Any& value)
{
    const FastName& v = value.Get<FastName>();
    if (v.IsValid() == false)
    {
        return String();
    }
    return String(v.c_str());
}

template <typename T>
String IntegralToString(const Any& value)
{
    return std::to_string(value.Get<T>());
}

template <typename T>
FastName IntegralToString(const Any& value)
{
    return FastName(std::to_string(value.Get<T>()));
}

String FilePathToString(const Any& value)
{
    return value.Get<FilePath>().GetAbsolutePathname();
}

FilePath StringToFilePath(const Any& value)
{
    return FilePath(value.Get<String>());
}

String Matrix2ToString(const Any& value)
{
    Matrix2 matrix = value.Get<Matrix2>();
    return Format("[%f, %f]\n[%f, %f]",
                  matrix._data[0][0], matrix._data[0][1],
                  matrix._data[1][0], matrix._data[1][1]);
}

String Matrix3ToString(const Any& value)
{
    Matrix3 matrix = value.Get<Matrix3>();
    return Format("[%f, %f, %f]\n[%f, %f, %f]\n[%f, %f, %f]",
                  matrix._data[0][0], matrix._data[0][1], matrix._data[0][2],
                  matrix._data[1][0], matrix._data[1][1], matrix._data[1][2],
                  matrix._data[2][0], matrix._data[2][1], matrix._data[2][2]
                  );
}

String Matrix4ToString(const Any& value)
{
    Matrix4 matrix = value.Get<Matrix4>();
    return Format("[%f, %f, %f, %f]\n[%f, %f, %f, %f]\n[%f, %f, %f, %f]\n[%f, %f, %f, %f]",
                  matrix._data[0][0], matrix._data[0][1], matrix._data[0][2], matrix._data[0][3],
                  matrix._data[1][0], matrix._data[1][1], matrix._data[1][2], matrix._data[1][3],
                  matrix._data[2][0], matrix._data[2][1], matrix._data[2][2], matrix._data[2][3],
                  matrix._data[3][0], matrix._data[3][1], matrix._data[3][2], matrix._data[3][3]
                  );
}

void RegisterAnyCasts()
{
    AnyCast<String, WideString>::Register(&StringToWideString);
    AnyCast<WideString, String>::Register(&WideStringToString);
    AnyCast<String, const char*>::Register(&StringToCharPointer);
    AnyCast<String, FastName>::Register(&StringToFastName);
    AnyCast<const char*, String>::Register(&CharPointerToString);
    AnyCast<const char*, FastName>::Register(&CharPointerToFastName);
    AnyCast<FastName, const char*>::Register(&FastNameToCharPointer);
    AnyCast<FastName, String>::Register(&FastNameToString);
    AnyCast<int32, size_t>::RegisterDefault();
    AnyCast<size_t, int32>::RegisterDefault();
    AnyCast<int8, String>::Register(&IntegralToString<int8>);
    AnyCast<uint8, String>::Register(&IntegralToString<uint8>);
    AnyCast<int16, String>::Register(&IntegralToString<int16>);
    AnyCast<uint16, String>::Register(&IntegralToString<uint16>);
    AnyCast<int32, String>::Register(&IntegralToString<int32>);
    AnyCast<uint32, String>::Register(&IntegralToString<uint32>);
    AnyCast<int64, String>::Register(&IntegralToString<int64>);
    AnyCast<uint64, String>::Register(&IntegralToString<uint64>);
    AnyCast<float32, String>::Register(&IntegralToString<float32>);
    AnyCast<float64, String>::Register(&IntegralToString<float64>);
    AnyCast<size_t, String>::Register(&IntegralToString<size_t>);
    AnyCast<int8, FastName>::Register(&IntegralToString<int8>);
    AnyCast<uint8, FastName>::Register(&IntegralToString<uint8>);
    AnyCast<int16, FastName>::Register(&IntegralToString<int16>);
    AnyCast<uint16, FastName>::Register(&IntegralToString<uint16>);
    AnyCast<int32, FastName>::Register(&IntegralToString<int32>);
    AnyCast<uint32, FastName>::Register(&IntegralToString<uint32>);
    AnyCast<int64, FastName>::Register(&IntegralToString<int64>);
    AnyCast<uint64, FastName>::Register(&IntegralToString<uint64>);
    AnyCast<size_t, FastName>::Register(&IntegralToString<size_t>);
    AnyCast<FilePath, String>::Register(&FilePathToString);
    AnyCast<String, FilePath>::Register(&StringToFilePath);
    AnyCast<float64, float32>::RegisterDefault();
    AnyCast<float32, float64>::RegisterDefault();
    AnyCast<int32, float32>::RegisterDefault();
    AnyCast<float32, int32>::RegisterDefault();
    AnyCast<int32, float64>::RegisterDefault();
    AnyCast<float64, int32>::RegisterDefault();
    AnyCast<uint32, int>::RegisterDefault();
    AnyCast<int, uint32>::RegisterDefault();
    AnyCast<uint16, int>::RegisterDefault();
    AnyCast<int, uint16>::RegisterDefault();
    AnyCast<uint8, int>::RegisterDefault();
    AnyCast<int, uint8>::RegisterDefault();
    AnyCast<int32, int>::RegisterDefault();
    AnyCast<int, int32>::RegisterDefault();
    AnyCast<int16, int>::RegisterDefault();
    AnyCast<int, int16>::RegisterDefault();
    AnyCast<int8, int>::RegisterDefault();
    AnyCast<int, int8>::RegisterDefault();
    AnyCast<eGPUFamily, int32>::RegisterDefault();
    AnyCast<int32, eGPUFamily>::RegisterDefault();

    AnyCast<Matrix2, String>::Register(&Matrix2ToString);
    AnyCast<Matrix3, String>::Register(&Matrix3ToString);
    AnyCast<Matrix4, String>::Register(&Matrix4ToString);
}

} // namespace DAVA
