#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
class FilePath;
class CRC32
{
public:
    CRC32();
    void AddData(const void* data, size_t size);
    uint32 Done();

    // Calculate the CRC32 for the in-memory buffer.
    static uint32 ForBuffer(const void* data, size_t size);

    template <typename Container>
    static uint32 ForBuffer(const Container& c)
    {
        return ForBuffer(c.data(), c.size());
    }

    // Calculate CRC32 for the whole file.
    static uint32 ForFile(const FilePath& pathName);

    // Calculate CRC32 for content of file without DVPL Footer
    static uint32 ForDVPLFileContent(const FilePath& pathName);

private:
    uint32 crc32;
};
}
