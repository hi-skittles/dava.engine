#include "ZLibStream.h"
#include "FileSystem/File.h"

namespace DAVA
{
ZLibIStream::ZLibIStream(File* _file)
    : file(_file)
{
    SafeRetain(file);

    zstream.zalloc = Z_NULL;
    zstream.zfree = Z_NULL;
    zstream.opaque = Z_NULL;
    zstream.avail_in = 0;
    zstream.next_in = Z_NULL;

    int res = inflateInit(&zstream);
    DVASSERT(Z_OK == res);
}

ZLibIStream::~ZLibIStream()
{
    inflateEnd(&zstream);
    SafeRelease(file);
}

uint32 ZLibIStream::Read(char8* data, uint32 size)
{
    zstream.avail_out = size;
    zstream.next_out = reinterpret_cast<unsigned char*>(data);

    while (zstream.avail_out > 0)
    {
        if (0 == zstream.avail_in)
        {
            zstream.avail_in = file->Read(readBuffer, ZLIB_CHUNK_SIZE);
            zstream.next_in = reinterpret_cast<unsigned char*>(readBuffer);
        }

        if (0 != zstream.avail_in)
        {
            if (Z_OK != inflate(&zstream, Z_NO_FLUSH))
            {
                break;
            }
        }
        else
        {
            // we didn't read anything
            break;
        }
    }

    return (size - zstream.avail_out);
}

ZLibOStream::ZLibOStream(File* _file, int compressionLevel)
    : file(_file)
{
    SafeRetain(file);

    zstream.zalloc = Z_NULL;
    zstream.zfree = Z_NULL;
    zstream.opaque = Z_NULL;
    zstream.avail_in = 0;
    zstream.next_in = Z_NULL;
    deflateInit(&zstream, compressionLevel);
}

ZLibOStream::~ZLibOStream()
{
    int ret = Z_OK;
    while (Z_OK == ret)
    {
        zstream.avail_out = ZLIB_CHUNK_SIZE;
        zstream.next_out = reinterpret_cast<unsigned char*>(writeBuffer);

        ret = deflate(&zstream, Z_FINISH);

        uint32 outSize = ZLIB_CHUNK_SIZE - zstream.avail_out;
        file->Write(writeBuffer, outSize);
    }

    deflateEnd(&zstream);
    SafeRelease(file);
}

uint32 ZLibOStream::Write(char8* data, uint32 size)
{
    zstream.avail_in = size;
    zstream.next_in = reinterpret_cast<unsigned char*>(data);

    while (zstream.avail_in > 0)
    {
        zstream.avail_out = ZLIB_CHUNK_SIZE;
        zstream.next_out = reinterpret_cast<unsigned char*>(writeBuffer);

        if (Z_OK == deflate(&zstream, Z_NO_FLUSH))
        {
            uint32 outSize = ZLIB_CHUNK_SIZE - zstream.avail_out;
            if (outSize != file->Write(writeBuffer, outSize))
            {
                // we didn't write everything
                break;
            }
        }
        else
        {
            break;
        }
    }

    return (size - zstream.avail_in);
}
}
