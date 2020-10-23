#ifndef __DAVAENGINE_TOOLS_ZLIB_STREAM_H__
#define __DAVAENGINE_TOOLS_ZLIB_STREAM_H__

#include "Base/BaseTypes.h"
#include "libpng/zlib.h"

namespace DAVA
{

#define ZLIB_CHUNK_SIZE 16384

class File;

class ZLibIStream
{
public:
    ZLibIStream(File* file);
    ~ZLibIStream();

    uint32 Read(char8* data, uint32 size);

protected:
    File* file;
    z_stream zstream;
    char8 readBuffer[ZLIB_CHUNK_SIZE];
    char8* next;
    uint32 available;
};

class ZLibOStream
{
public:
    ZLibOStream(File* file, int compressionLevel = Z_BEST_COMPRESSION);
    ~ZLibOStream();

    uint32 Write(char8* data, uint32 size);

protected:
    File* file;
    z_stream zstream;
    char8 writeBuffer[ZLIB_CHUNK_SIZE];
};
}

#endif // __DAVAENGINE_TOOLS_ZLIB_STREAM_H__
