#include "FileSystem/KeyedUnarchiver.h"
#include "Utils/Utils.h"

namespace DAVA
{
KeyedUnarchiver::KeyedUnarchiver()
{
}
KeyedUnarchiver::~KeyedUnarchiver()
{
}

bool KeyedUnarchiver::UnarchiveFile(const FilePath& pathName)
{
    File* archive = File::Create(pathName, File::OPEN | File::READ);
    if (!archive)
        return false;

    UnarchiveFile(archive);

    SafeRelease(archive);
    return true;
}

bool KeyedUnarchiver::UnarchiveFile(File* file)
{
    while (!file->IsEof())
    {
        VariantType key;
        key.Read(file);
        VariantType value;
        value.Read(file);
        objectMap[key.AsString()] = value;
    }
    return true;
}

bool KeyedUnarchiver::IsKeyExists(const String& key)
{
    Map<String, VariantType>::iterator t = objectMap.find(key);
    if (t != objectMap.end())
    {
        return true;
    }
    return false;
}

bool KeyedUnarchiver::DecodeBool(const String& key)
{
    return objectMap[key].AsBool();
}

int32 KeyedUnarchiver::DecodeInt(const String& key)
{
    return objectMap[key].AsInt32();
}

float32 KeyedUnarchiver::DecodeFloat(const String& key)
{
    return objectMap[key].AsFloat();
}

const String& KeyedUnarchiver::DecodeString(const String& key)
{
    return objectMap[key].AsString();
}
// internally we always use utf8 in std::string, so can't return ref
const WideString KeyedUnarchiver::DecodeWideString(const String& key)
{
    return objectMap[key].AsWideString();
}

const VariantType& KeyedUnarchiver::DecodeVariant(const String& key)
{
    return objectMap[key];
}
};
