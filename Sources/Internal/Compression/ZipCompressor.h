#ifndef COMPRESSION_ZIP_COMPRESSOR_H
#define COMPRESSION_ZIP_COMPRESSOR_H

#include "Compression/Compressor.h"
#include "Base/RefPtr.h"

namespace DAVA
{
class FilePath;
class ZipPrivateData;
class File;

// deflate/inflate
class ZipCompressor : public Compressor
{
public:
    bool Compress(const Vector<uint8>& in, Vector<uint8>& out) const override;
    // you should resize output to correct size before call this method
    bool Decompress(const Vector<uint8>& in, Vector<uint8>& out) const override;
};

class ZipFile final
{
public:
    ZipFile(RefPtr<File>& file_, const FilePath& path);
    ~ZipFile();

    uint32 GetNumFiles() const;
    DAVA::int32 GetFileIndex(const String& relativeFilePath) const;
    bool Exists(const String& relativeFilePath) const;
    bool GetFileInfo(uint32 fileIndex, String& relativeFilePath, uint32& fileOriginalSize, uint32& fileCompressedSize, bool& isDirectory) const;

    bool LoadFile(const String& relativeFilePath, Vector<uint8>& fileContent) const;
    bool SaveFile(const String& relativeFilePath, Vector<uint8>& fileContent) const;

private:
    void StartWrite() const;
    void StartRead() const;
    void Finalize() const;

    std::unique_ptr<ZipPrivateData> zipData;
};

} // end namespace DAVA

#endif // COMPRESSION_ZIP_COMPRESSOR_H