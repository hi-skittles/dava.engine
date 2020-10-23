#ifndef __DAVAENGINE_KEYED_ARCHIVER_H__
#define __DAVAENGINE_KEYED_ARCHIVER_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "FileSystem/VariantType.h"
#include "FileSystem/File.h"

namespace DAVA
{
/*
	this class is DEPRECATED. If you want to use archieving or unarchiving use KeyedArchive instead of this class.
 */
class KeyedArchiver : public BaseObject
{
protected:
    virtual ~KeyedArchiver();

public:
    KeyedArchiver();

    bool StartEncodingToFile(const FilePath& pathName);
    bool StartEncodingToFile(File* file);

    void EncodeBool(const String& key, bool value);
    void EncodeInt32(const String& key, int32 value);
    void EncodeFloat(const String& key, float32 value);
    void EncodeString(const String& key, const String& value);
    void EncodeWideString(const String& key, const WideString& value);
    void EncodeVariant(const String& key, const VariantType& value);

    void FinishEncoding();

private:
    File* archive;
};
};

#endif // __DAVAENGINE_KEYED_ARCHIVER_H__