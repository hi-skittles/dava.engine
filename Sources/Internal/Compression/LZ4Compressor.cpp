#include "Compression/LZ4Compressor.h"
#include "Logger/Logger.h"

#include <lz4/lz4.h>
#include <lz4/lz4hc.h>

namespace DAVA
{
Compressor::~Compressor() = default; // only one virtual table(fix warning)

bool LZ4Compressor::Compress(const Vector<uint8>& in, Vector<uint8>& out) const
{
    if (in.size() > LZ4_MAX_INPUT_SIZE)
    {
        Logger::Error("LZ4 compress failed too big input buffer");
        return false;
    }
    uint32 maxSize = static_cast<uint32>(LZ4_compressBound(static_cast<uint32>(in.size())));
    if (out.size() < maxSize)
    {
        out.resize(maxSize);
    }
    int32 compressedSize = LZ4_compress(reinterpret_cast<const char*>(in.data()), reinterpret_cast<char*>(out.data()), static_cast<uint32>(in.size()));
    if (compressedSize == 0)
    {
        return false;
    }
    out.resize(static_cast<uint32>(compressedSize));
    return true;
}

bool LZ4Compressor::Decompress(const Vector<uint8>& in, Vector<uint8>& out) const
{
    int32 decompressResult = LZ4_decompress_fast(reinterpret_cast<const char*>(in.data()), reinterpret_cast<char*>(out.data()), static_cast<uint32>(out.size()));
    if (decompressResult < 0)
    {
        Logger::Error("LZ4 decompress failed");
        return false;
    }
    return true;
}

bool LZ4HCCompressor::Compress(const Vector<uint8>& in, Vector<uint8>& out) const
{
    if (in.size() > LZ4_MAX_INPUT_SIZE)
    {
        Logger::Error("LZ4 compress failed too big input buffer");
        return false;
    }
    if (in.empty())
    {
        Logger::Error("LZ4 can't compress empty buffer");
        return false;
    }
    uint32 maxSize = static_cast<uint32>(LZ4_compressBound(static_cast<uint32>(in.size())));
    if (out.size() < maxSize)
    {
        out.resize(maxSize);
    }
    int32 compressedSize = LZ4_compressHC(reinterpret_cast<const char*>(in.data()), reinterpret_cast<char*>(out.data()), static_cast<uint32>(in.size()));
    if (compressedSize == 0)
    {
        return false;
    }
    out.resize(static_cast<uint32>(compressedSize));
    return true;
}

} // end namespace DAVA
