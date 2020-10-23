#include "archive.h"

#include "lite_archive.h"
#include "pack_archive.h"

Archive* Create(const std::string& file)
{
    // read last 4 bytes of file
    std::ifstream f(file, std::ios_base::binary);

    f.seekg(-4, std::ios_base::end);

    std::array<char, 4> marker;

    f.read(&marker[0], 4);

    if (marker == pack_format::file_marker)
    {
        return new PackArchive(file);
    }
    if (marker == pack_format::file_marker_lite)
    {
        return new LiteArchive(file);
    }
    throw std::runtime_error(std::string("can't open file, marker = ") + marker[0] + marker[1] + marker[2] + marker[3]);
}
