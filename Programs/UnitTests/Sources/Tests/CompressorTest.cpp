#include <Compression/ZipCompressor.h>
#include <Compression/LZ4Compressor.h>

#include "UnitTests/UnitTests.h"

using namespace DAVA;

DAVA_TESTCLASS (CompressorTest)
{
    DAVA_TEST (TestLZ4_LZ4HC_ZIP)
    {
        uint32 inSize = 4096;
        const Vector<uint8> in(inSize, 'a');

        {
            LZ4Compressor lz4;

            Vector<uint8> compressedLz4;

            TEST_VERIFY(lz4.Compress(in, compressedLz4));

            TEST_VERIFY(in.size() > compressedLz4.size());

            Vector<uint8> uncompressedLz4(inSize, '\0');

            TEST_VERIFY(lz4.Decompress(compressedLz4, uncompressedLz4));

            TEST_VERIFY(uncompressedLz4 == in);
        }

        {
            LZ4HCCompressor lz4hc;

            Vector<uint8> compressedLz4hc;

            TEST_VERIFY(lz4hc.Compress(in, compressedLz4hc));

            TEST_VERIFY(in.size() > compressedLz4hc.size());

            Vector<uint8> uncompressedLz4hc(inSize, '\0');

            TEST_VERIFY(lz4hc.Decompress(compressedLz4hc, uncompressedLz4hc));

            TEST_VERIFY(uncompressedLz4hc == in);
        }

        {
            ZipCompressor zip;

            Vector<uint8> compressedDeflate;

            TEST_VERIFY(zip.Compress(in, compressedDeflate));

            TEST_VERIFY(in.size() > compressedDeflate.size());

            Vector<uint8> uncompressedZip(inSize, '\0');

            TEST_VERIFY(zip.Decompress(compressedDeflate, uncompressedZip));

            TEST_VERIFY(uncompressedZip == in);
        }
    }
};
