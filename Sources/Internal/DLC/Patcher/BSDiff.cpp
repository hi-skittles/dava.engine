#include "BSDiff.h"
#include "ZLibStream.h"
#include "FileSystem/File.h"

namespace DAVA
{
bool BSDiff::Diff(char8* origData, uint32 origSize, char8* newData, uint32 newSize, File* patchFile, BSType type)
{
    bool ret = false;
    ZLibOStream outStream(patchFile);

    if (NULL != patchFile)
    {
        // write BS type
        uint32 typeToWrite = type;
        patchFile->Write(&typeToWrite);

        bsdiff_stream diffStream;
        diffStream.type = type;
        diffStream.free = &BSDiff::BSFree;
        diffStream.malloc = &BSDiff::BSMalloc;
        diffStream.write = &BSDiff::BSWrite;

        switch (type)
        {
        case BS_ZLIB:
            diffStream.opaque = &outStream;
            break;
        case BS_PLAIN:
            diffStream.opaque = patchFile;
            break;
        default:
            DVASSERT(0 && "Unknow BS-type");
            break;
        }

        // make bsdiff
        if (0 == bsdiff(reinterpret_cast<uint8_t*>(origData), origSize, reinterpret_cast<uint8_t*>(newData), newSize, &diffStream))
        {
            ret = true;
        }
    }

    return ret;
}

// This function should be as safe as possible.
// So we should continue to work event after DVASSERT
bool BSDiff::Patch(char8* origData, uint32 origSize, char8* newData, uint32 newSize, File* patchFile)
{
    bool ret = false;
    ZLibIStream inStream(patchFile);

    // read BS type
    uint32 typeToRead = -1;
    if (sizeof(typeToRead) == patchFile->Read(&typeToRead))
    {
        bool type_is_ok = true;
        bspatch_stream patchStream;
        patchStream.read = &BSDiff::BSRead;
        patchStream.type = static_cast<BSType>(typeToRead);

        switch (typeToRead)
        {
        case BS_ZLIB:
            patchStream.opaque = &inStream;
            break;
        case BS_PLAIN:
            patchStream.opaque = patchFile;
            break;
        default:
            DVASSERT(0 && "Unknow BS-type");
            type_is_ok = false;
            break;
        }

        if (type_is_ok)
        {
            // apply bsdiff
            if (0 == bspatch(reinterpret_cast<uint8_t*>(origData), origSize, reinterpret_cast<uint8_t*>(newData), newSize, &patchStream))
            {
                ret = true;
            }
        }
    }

    return ret;
}

void* BSDiff::BSMalloc(int64_t size)
{
    return new uint8_t[static_cast<size_t>(size)];
}

void BSDiff::BSFree(void* ptr)
{
    if (NULL != ptr)
    {
        delete[] static_cast<uint8_t*>(ptr);
    }
}

int BSDiff::BSWrite(struct bsdiff_stream* stream, const void* buffer, int64_t size)
{
    int ret = 0;

    if (stream->type == BS_PLAIN)
    {
        File* file = static_cast<File*>(stream->opaque);
        if (size != file->Write(static_cast<const char8*>(buffer), static_cast<uint32>(size)))
        {
            ret = -1;
        }
    }
    else if (stream->type == BS_ZLIB)
    {
        ZLibOStream* outStream = static_cast<ZLibOStream*>(stream->opaque);
        void* nonConstBuffer = const_cast<void*>(buffer);
        if (size != outStream->Write(static_cast<char8*>(nonConstBuffer), static_cast<uint32>(size)))
        {
            ret = -1;
        }
    }
    else
    {
        DVASSERT(0 && "Unknow BS-type");
        ret = -1;
    }

    return ret;
}

// This function should be as safe as possible.
// So we should continue to work event after DVASSERT
int BSDiff::BSRead(const struct bspatch_stream* stream, void* buffer, int64_t size)
{
    int ret = 0;

    if (stream->type == BS_PLAIN)
    {
        File* file = static_cast<File*>(stream->opaque);
        if (size != file->Read(static_cast<char8*>(buffer), static_cast<uint32>(size)))
        {
            ret = -1;
        }
    }
    else if (stream->type == BS_ZLIB)
    {
        ZLibIStream* inStream = static_cast<ZLibIStream*>(stream->opaque);
        if (size != inStream->Read(static_cast<char8*>(buffer), static_cast<uint32>(size)))
        {
            ret = -1;
        }
    }
    else
    {
        DVASSERT(0 && "Unknow BS-type");
        ret = -1;
    }

    return ret;
}
}