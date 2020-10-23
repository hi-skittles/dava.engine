#ifndef __DAVAENGINE_MEMPROFILERTYPES_H__
#define __DAVAENGINE_MEMPROFILERTYPES_H__

#include "Base/BaseTypes.h"

namespace DAVA
{
/*
 Most of structs declared inside namespace MemMgr can be transferred over network or saved to file
 so theirs size and layout must be the same on all platforms. It's achieved by selecting
 appropiate data types, layouting members and inserting necessary padding. Also static size
 check is performed.
*/

/*
 TagAllocStat - memory statistics for memory tag
*/
struct TagAllocStat
{
    uint32 allocByApp; // Number of bytes allocated by application
    uint32 blockCount; // Number of allocated blocks
};
static_assert(sizeof(TagAllocStat) == 8, "sizeof(TagAllocStat) != 8");

/*
 AllocPoolStat - memory statistics calculated for every allocation pool
*/
struct AllocPoolStat
{
    uint32 allocByApp; // Number of bytes allocated by application
    uint32 allocTotal; // Total allocated bytes: allocByApp + overhead imposed by memory manager and aligning
    uint32 blockCount; // Number of allocated blocks
    uint32 maxBlockSize; // Max allocated block size
};
static_assert(sizeof(AllocPoolStat) % 16 == 0, "sizeof(AllocPoolStat) % 16 != 0");

/*
 GeneralAllocStat - general memory statistics
*/
struct GeneralAllocStat
{
    uint32 nextBlockNo; // Order number which will be assigned to next allocated memory block
    uint32 activeTags; // Current active tags
    uint32 activeTagCount; // Number of active tags
    uint32 allocInternal; // Size of memory allocated for memory manager internal use: symbol table, etc
    uint32 internalBlockCount; // Number of internal memory blocks
    uint32 ghostBlockCount; // Number of blocks allocated bypassing memory manager
    uint32 ghostSize; // Size of bypassed memory
    uint32 allocInternalTotal;
};
static_assert(sizeof(GeneralAllocStat) % 16 == 0, "sizeof(GeneralAllocStat) % 16 != 0");

/*
 MMItemName is used to store name of tag, allocation pool, etc
*/
struct MMItemName
{
    static const size_t MAX_NAME_LENGTH = 16;
    char8 name[MAX_NAME_LENGTH];
};
static_assert(sizeof(MMItemName) % 16 == 0, "sizeof(MMItemName) % 16 != 0");

/*
 MMBacktrace is used to store stack frames
 Layout after this header:
    uint64 frames[MemoryManager::BACKTRACE_DEPTH] - stack frames
*/
struct MMBacktrace
{
    uint32 hash; // Backtrace hash
    uint32 padding;
    // uint64 frames[];
};
static_assert(sizeof(MMBacktrace) == 8, "sizeof(MMBacktrace) != 8");

/*
 MMSymbol is used to transfer symbol name
*/
struct MMSymbol
{
    static const size_t NAME_LENGTH = 136; // Reasons to select 136 as name length:
    //  - make struct size to be multiple of 16
    //  - allow to store long enough symbol name
    uint64 addr;
    char8 name[NAME_LENGTH];
};
static_assert(sizeof(MMSymbol) % 16 == 0, "sizeof(MMSymbol) % 16 == 0");

/*
 MMBlock is used to transfer symbol name
*/
struct MMBlock
{
    uint32 orderNo; // Block order number
    uint32 allocByApp; // Size requested by application
    uint32 allocTotal; // Total allocated size
    uint32 bktraceHash; // Unique hash number to identify block backtrace
    uint32 pool; // Allocation pool block belongs to
    uint32 tags; // Tags block belongs to
    uint32 type;
    uint32 padding;
};
static_assert(sizeof(MMBlock) % 16 == 0, "sizeof(MMBlock) % 16 == 0");

//////////////////////////////////////////////////////////////////////////

/*
 MMStatConfig contains information about memory manager configuration.
 Layout after this header:
    MMItemName allocPoolNames[allocPoolCount] - names of registered allocation pools
    MMItemName tagNames[tagCount]             - names of all tags including empty and unused
*/
struct MMStatConfig
{
    uint32 size; // Total size of configuration
    uint32 allocPoolCount; // Number of registered allocation pools
    uint32 tagCount; // Number of registered tags
    uint32 bktraceDepth; // Depth of collected backtrace
    // MMItemName allocPoolNames[];
    // MMItemName tagNames[];
};
static_assert(sizeof(MMStatConfig) == 16, "sizeof(MMStatConfig) != 16");

/*
 MMCurStat represents current memory allocation statistics
 Layout after this header:
    AllocPoolStat statAllocPool[] - allocation statistics by pools, size StatConfigHdr::allocPoolCount
    TagAllocStat statTagAlloc[]   - allocation statistics by tags, size StatConfigHdr::tagCount
*/
struct MMCurStat
{
    uint64 timestamp; // Room for timestamp, not filled by memory manager
    uint32 size; // Total size of allocation statistics
    uint32 padding;
    GeneralAllocStat statGeneral; // General statistics
    // AllocPoolStat statAllocPool[];
    // TagAllocStat statTagAlloc[];
};
static_assert(sizeof(MMCurStat) % 16 == 0, "sizeof(MMCurStat) % 16 == 0");

/*
 MMSnapshot represents memory snapshot
 Layout after this header:
    MMCurStat statCur                 - current memory allocation statistics with its layout
    MMBlock blocks[blockCount]        - memory blocks
    MMSymbol symbols[symbolCount]     - symbols
    MMBacktrace bktrace[bktraceCount] - backtraces
 Backtrace array size calculation formula:
    size = (sizeof(MMBacktrace) + bktraceDepth * sizeof(uint64)) * bktraceCount;
*/
struct MMSnapshot
{
    uint64 timestamp; // Room for timestamp
    uint32 size; // Total size of memory snapshot
    uint32 dataOffset; // Offset of snapshot data
    uint32 blockCount; // Number of blocks in snapshot
    uint32 bktraceCount; // Number of backtraces in snapshot
    uint32 symbolCount; // Number of symbols in snapshot
    uint32 bktraceDepth; // Depth of collected backtrace
    // MMBlock blocks[];
    // MMSymbol symbols[];
    // MMBacktrace bktrace[];
};
static_assert(sizeof(MMSnapshot) % 16 == 0, "sizeof(MMSnapshot) % 16 == 0");

} // namespace DAVA

#endif // __DAVAENGINE_MEMPROFILERTYPES_H__
