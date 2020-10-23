#ifndef __DAVAENGINE_KEYED_UNARCHIVER_H__
#define __DAVAENGINE_KEYED_UNARCHIVER_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "FileSystem/VariantType.h"
#include "FileSystem/File.h"

namespace DAVA
{
/*
	this class is DEPRECATED. If you want to use archieving or unarchiving use KeyedArchive instead of this class.
 */
class KeyedUnarchiver : public BaseObject
{
protected:
    virtual ~KeyedUnarchiver();

public:
    KeyedUnarchiver();

    bool UnarchiveFile(const FilePath& pathName);
    bool UnarchiveFile(File* file);

    bool IsKeyExists(const String& key);

    bool DecodeBool(const String& key);
    int32 DecodeInt(const String& key);
    float32 DecodeFloat(const String& key);
    const String& DecodeString(const String& key);
    const WideString DecodeWideString(const String& key);
    const VariantType& DecodeVariant(const String& key);

private:
    Map<String, VariantType> objectMap;
};
};

#endif // __DAVAENGINE_KEYED_UNARCHIVER_H__