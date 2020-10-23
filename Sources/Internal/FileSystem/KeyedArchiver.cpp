#include "FileSystem/KeyedArchiver.h"
#include "Utils/Utils.h"

namespace DAVA
{
KeyedArchiver::KeyedArchiver()
    : archive(0)
{
}

KeyedArchiver::~KeyedArchiver()
{
}

bool KeyedArchiver::StartEncodingToFile(const FilePath& pathName)
{
    archive = File::Create(pathName, File::CREATE | File::WRITE);
    if (!archive)
        return false;
    return true;
}

bool KeyedArchiver::StartEncodingToFile(File* file)
{
    archive = SafeRetain(file);
    return true;
}

void KeyedArchiver::EncodeBool(const String& key, bool value)
{
    VariantType keyMT;
    keyMT.SetString(key);
    keyMT.Write(archive);

    VariantType valueMT;
    valueMT.SetBool(value);
    valueMT.Write(archive);
}

void KeyedArchiver::EncodeInt32(const String& key, int32 value)
{
    VariantType keyMT;
    keyMT.SetString(key);
    keyMT.Write(archive);

    VariantType valueMT;
    valueMT.SetInt32(value);
    valueMT.Write(archive);
}

void KeyedArchiver::EncodeFloat(const String& key, float32 value)
{
    VariantType keyMT;
    keyMT.SetString(key);
    keyMT.Write(archive);

    VariantType valueMT;
    valueMT.SetFloat(value);
    valueMT.Write(archive);
}

void KeyedArchiver::EncodeString(const String& key, const String& value)
{
    VariantType keyMT;
    keyMT.SetString(key);
    keyMT.Write(archive);

    VariantType valueMT;
    valueMT.SetString(value);
    valueMT.Write(archive);
}

void KeyedArchiver::EncodeWideString(const String& key, const WideString& value)
{
    VariantType keyMT;
    keyMT.SetString(key);
    keyMT.Write(archive);

    VariantType valueMT;
    valueMT.SetWideString(value);
    valueMT.Write(archive);
}

void KeyedArchiver::EncodeVariant(const String& key, const VariantType& value)
{
    VariantType keyMT;
    keyMT.SetString(key);
    keyMT.Write(archive);

    value.Write(archive);
}

void KeyedArchiver::FinishEncoding()
{
    SafeRelease(archive);
}
}