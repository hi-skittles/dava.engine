#ifndef __DAVAENGINE_TOOLS_DIFF_H__
#define __DAVAENGINE_TOOLS_DIFF_H__

#include "Base/BaseTypes.h"
#include "bsdiff/bs_common.h"

namespace DAVA
{
class File;

class BSDiff
{
public:
    static bool Diff(char8* origData, uint32 origSize, char8* newData, uint32 newSize, File* patchFile, BSType type);
    static bool Patch(char8* origData, uint32 origSize, char8* newData, uint32 newSize, File* patchFile);

protected:
    static void* BSMalloc(int64_t size);
    static void BSFree(void* ptr);
    static int BSWrite(struct bsdiff_stream* stream, const void* buffer, int64_t size);
    static int BSRead(const struct bspatch_stream* stream, void* buffer, int64_t size);
};
}

#endif // __DAVAENGINE_TOOLS_DIFF_H__