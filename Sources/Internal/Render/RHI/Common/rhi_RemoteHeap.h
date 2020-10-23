#pragma once

#include "../Common/rhi_Utils.h"

namespace rhi
{
class SimpleRemoteHeap
{
public:
    struct Stat
    {
        uint32 reserved;
        uint32 comitted_size;
        uint32 alloced_size;
        uint32 alloced_block_count;
        uint32 total_block_count;
    };

public:
    SimpleRemoteHeap(uint32 maxBlockCount);
    ~SimpleRemoteHeap() = default;

    void initialize(void* base, uint32 size, uint32 align = 8);

    void* alloc(uint32 sz);
    void* alloc_aligned(uint32 sz, uint32 align);
    void free(void* mem);

    bool Empty();
    void get_stats(Stat* stat);

private:
    SimpleRemoteHeap(SimpleRemoteHeap&&) = delete;
    SimpleRemoteHeap(const SimpleRemoteHeap&) = delete;
    SimpleRemoteHeap& operator=(const SimpleRemoteHeap&) = delete;

private:
    struct Block
    {
        enum : uint32
        {
            Allocated = 0x00000001,
        };

        uint8_t* base; // 'real' address
        uint8_t* usr_ptr; // ponter given outside of allocator
        uint32_t size; // 'real', aligned size
        uint32_t usr_sz;
        uint32_t align;
        uint32_t slack;
        uint32_t flags;

        bool IsAllocated() const
        {
            return flags & Allocated;
        }

        void mark_alloced()
        {
            flags |= Allocated;
        }

        void mark_not_alloced()
        {
            flags &= ~Allocated;
        }
    };

    Block* _commit_block(uint32 sz, uint32 align);
    Block* _find_free_block(uint32 sz, uint32 align, unsigned* slack = nullptr);
    void _defragment();

private:
    std::vector<Block> _block;
    uint8_t* _base = nullptr;
    uint8_t* _unused = nullptr;
    uint32 _total_size = 0;
    uint32 _block_align = 0;
    uint32 maxBlockCount = 0;
};

inline SimpleRemoteHeap::SimpleRemoteHeap(uint32 maxBlocks)
    : maxBlockCount(maxBlocks)
{
    _block.reserve(maxBlockCount);
}

inline void SimpleRemoteHeap::initialize(void* base, uint32 size, uint32 align)
{
    _base = reinterpret_cast<uint8_t*>(base);
    _unused = reinterpret_cast<uint8_t*>(base);
    _total_size = size;
    _block_align = align;
}

inline void* SimpleRemoteHeap::alloc(uint32 size)
{
    return alloc_aligned(size, _block_align);
}

inline void* SimpleRemoteHeap::alloc_aligned(uint32 size, uint32 align)
{
    _defragment();

    uint8_t* mem = nullptr;
    uint32 slack = 0;
    Block* block = _find_free_block(size, align, &slack);

    if (block)
        block->slack = slack;
    else
        block = _commit_block(size, align);

    if (block)
    {
        mem = block->usr_ptr;
        block->mark_alloced();
    }

    return mem;
}

inline void SimpleRemoteHeap::free(void* mem)
{
    uint8_t* usr_ptr = reinterpret_cast<uint8_t*>(mem);
    DVASSERT(usr_ptr >= _base);
    DVASSERT(usr_ptr < _base + _total_size);

    for (Block& b : _block)
    {
        if (b.usr_ptr == usr_ptr)
        {
            DVASSERT(b.IsAllocated());
            b.mark_not_alloced();
            b.slack = 0;
            break;
        }
    }
}

inline SimpleRemoteHeap::Block* SimpleRemoteHeap::_commit_block(uint32 size, uint32 align)
{
    DVASSERT(_base);

    Block* block = nullptr;
    uint32 sz = L_ALIGNED_SIZE(size, align);

    if ((_unused + sz <= _base + _total_size) && (_block.size() < maxBlockCount))
    {
        _block.emplace_back();

        Block& b = _block.back();
        b.base = _unused;
        b.size = sz;
        b.align = align;
        b.usr_ptr = reinterpret_cast<uint8_t*>((reinterpret_cast<uint64_t>(b.base) + (uint64_t(align) - 1)) & (~(uint64_t(align) - 1)));
        b.usr_sz = size;
        b.slack = 0;
        b.flags = 0;
        block = &b;

        _unused += sz;
    }

    return block;
}

inline SimpleRemoteHeap::Block* SimpleRemoteHeap::_find_free_block(uint32 size, uint32 align, unsigned* wasted_slack)
{
    auto block = _block.end();
    uint32 min_slack = size;

    uint32 sz = L_ALIGNED_SIZE(size, align);

    for (auto b = _block.begin(), b_end = _block.end(); b != b_end; ++b)
    {
        if (!b->IsAllocated() && b->usr_sz >= sz)
        {
            uint32 slack = b->usr_sz - size;

            if (slack < min_slack)
            {
                block = b;
                min_slack = slack;
            }
        }
    }

    if (block != _block.end())
    {
        if (wasted_slack)
            *wasted_slack = min_slack;
    }

    return (block != _block.end()) ? &(_block[block - _block.begin()]) : nullptr;
}

inline void SimpleRemoteHeap::_defragment()
{
    bool has_free_blk = false;
    bool do_wipe = true;

    for (const Block& b : _block)
    {
        if (b.IsAllocated())
        {
            do_wipe = false;
            break;
        }
    }

    if (do_wipe)
    {
        _block.clear();
        _unused = _base;
        return;
    }

    do
    {
        uint8_t* max_ptr = _base;
        auto blk = _block.end();

        for (auto b = _block.begin(), b_end = _block.end(); b != b_end; ++b)
        {
            if (b->base > max_ptr)
            {
                blk = b;
                max_ptr = b->base;
            }
        }

        if ((blk != _block.end()) && !blk->IsAllocated())
        {
            // de-commit block's memory
            _unused -= blk->size;
            _block.erase(blk);
            has_free_blk = true;
        }
        else
        {
            has_free_blk = false;
        }
    }
    while (has_free_blk);
}

inline bool SimpleRemoteHeap::Empty()
{
    bool isEmpty = true;

    for (const Block& b : _block)
    {
        if (b.IsAllocated())
        {
            isEmpty = false;
            break;
        }
    }

    return isEmpty;
}

inline void SimpleRemoteHeap::get_stats(Stat* stat)
{
    uint32 wasted_align = 0;
    uint32 wasted_slack = 0;
    uint32 alloced = 0;

    stat->alloced_size = 0;
    stat->alloced_block_count = 0;

    for (const Block& b : _block)
    {
        wasted_align += b.size - b.usr_sz;
        wasted_slack += b.slack;

        if (b.IsAllocated())
        {
            stat->alloced_size += b.usr_sz;
            ++stat->alloced_block_count;
        }
    }

    stat->reserved = _total_size;
    stat->comitted_size = static_cast<uint32>(_unused - _base);
    stat->total_block_count = static_cast<uint32>(_block.size());
}
}
