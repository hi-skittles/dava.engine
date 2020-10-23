#pragma once

#include "Compression/Compressor.h"

namespace DAVA
{
class LZ4Compressor : public Compressor
{
public:
    bool Compress(const Vector<uint8>& in, Vector<uint8>& out) const override;
    // you should resize output to correct size before call this method
    bool Decompress(const Vector<uint8>& in, Vector<uint8>& out) const override;
};

class LZ4HCCompressor final : public LZ4Compressor
{
public:
    bool Compress(const Vector<uint8>& in, Vector<uint8>& out) const override;
};

} // end namespace DAVA
