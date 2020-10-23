#include "../Common/rhi_Private.h"
    #include "../Common/rhi_Pool.h"
    #include "rhi_DX9.h"

    #include "Debug/DVAssert.h"
    #include "Logger/Logger.h"
using DAVA::Logger;
#include "Utils/Utils.h"

    #include "_dx9.h"

namespace rhi
{
//==============================================================================

class
QueryBufferDX9_t
{
public:
    QueryBufferDX9_t()
        : curObjectIndex(DAVA::InvalidIndex)
        , bufferCompleted(false){};
    ~QueryBufferDX9_t(){};

    std::vector<std::pair<IDirect3DQuery9*, uint32>> pendingQueries;
    std::vector<uint32> results;
    uint32 curObjectIndex;
    uint32 bufferCompleted : 1;
};

typedef ResourcePool<QueryBufferDX9_t, RESOURCE_QUERY_BUFFER, QueryBuffer::Descriptor, false> QueryBufferDX9Pool;
RHI_IMPL_POOL(QueryBufferDX9_t, RESOURCE_QUERY_BUFFER, QueryBuffer::Descriptor, false);

std::vector<IDirect3DQuery9*> QueryDX9Pool;
std::vector<QueryBufferDX9_t*> queryBuffers;

#define DX9_MAX_PENDING_QUERIES 256

//==============================================================================

static Handle dx9_QueryBuffer_Create(uint32 maxObjectCount)
{
    Handle handle = QueryBufferDX9Pool::Alloc();
    QueryBufferDX9_t* buf = QueryBufferDX9Pool::Get(handle);
    DVASSERT(buf);

    queryBuffers.push_back(buf);

    buf->results.resize(maxObjectCount);
    memset(buf->results.data(), 0, sizeof(uint32) * buf->results.size());
    buf->pendingQueries.clear();
    buf->curObjectIndex = DAVA::InvalidIndex;
    buf->bufferCompleted = false;

    return handle;
}

static void dx9_QueryBuffer_Delete(Handle handle)
{
    QueryBufferDX9_t* buf = QueryBufferDX9Pool::Get(handle);
    DVASSERT(buf);

    DAVA::FindAndRemoveExchangingWithLast(queryBuffers, buf);

    if (buf->pendingQueries.size())
    {
        for (size_t q = 0; q < buf->pendingQueries.size(); ++q)
            QueryDX9Pool.push_back(buf->pendingQueries[q].first);

        buf->pendingQueries.clear();
    }

    QueryBufferDX9Pool::Free(handle);
}

static void dx9_QueryBuffer_Reset(Handle handle)
{
    QueryBufferDX9_t* buf = QueryBufferDX9Pool::Get(handle);
    DVASSERT(buf);

    memset(buf->results.data(), 0, sizeof(uint32) * buf->results.size());

    if (buf->pendingQueries.size())
    {
        for (size_t q = 0; q < buf->pendingQueries.size(); ++q)
            QueryDX9Pool.push_back(buf->pendingQueries[q].first);

        buf->pendingQueries.clear();
    }

    buf->curObjectIndex = DAVA::InvalidIndex;
    buf->bufferCompleted = false;
}

static void dx9_Check_Query_Results(QueryBufferDX9_t* buf)
{
    DX9Command cmd[DX9_MAX_PENDING_QUERIES];
    DWORD results[DX9_MAX_PENDING_QUERIES] = {};
    uint32 cmdCount = uint32(buf->pendingQueries.size());

    if (cmdCount)
    {
        DVASSERT(cmdCount < DX9_MAX_PENDING_QUERIES);

        for (uint32 q = 0; q < cmdCount; ++q)
        {
            cmd[q] = { DX9Command::GET_QUERY_DATA, { uint64_t(buf->pendingQueries[q].first), uint64_t(&results[q]), sizeof(DWORD), 0 } }; // DO NOT flush
        }

        ExecDX9(cmd, cmdCount, false);

        for (int32 q = cmdCount - 1; q >= 0; --q)
        {
            uint32 resultIndex = buf->pendingQueries[q].second;
            if (cmd[q].retval == S_OK)
            {
                if (resultIndex < uint32(buf->results.size()))
                    buf->results[resultIndex] += results[q];

                QueryDX9Pool.push_back(buf->pendingQueries[q].first);

                buf->pendingQueries[q] = buf->pendingQueries.back();
                buf->pendingQueries.pop_back();
            }
        }
    }
}

static bool dx9_QueryBuffer_IsReady(Handle handle)
{
    bool ready = false;
    QueryBufferDX9_t* buf = QueryBufferDX9Pool::Get(handle);
    DVASSERT(buf);

    if (buf->bufferCompleted)
    {
        dx9_Check_Query_Results(buf);
        ready = (buf->pendingQueries.size() == 0);
    }

    return ready;
}

static bool dx9_QueryBuffer_ObjectIsReady(Handle handle, uint32 objectIndex)
{
    bool ready = false;
    QueryBufferDX9_t* buf = QueryBufferDX9Pool::Get(handle);
    DVASSERT(buf);

    if (buf->bufferCompleted)
    {
        dx9_Check_Query_Results(buf);

        ready = true;
        for (size_t q = 0; q < buf->pendingQueries.size(); ++q)
        {
            if (buf->pendingQueries[q].second == objectIndex)
            {
                ready = false;
                break;
            }
        }
    }

    return ready;
}

static int32 dx9_QueryBuffer_Value(Handle handle, uint32 objectIndex)
{
    QueryBufferDX9_t* buf = QueryBufferDX9Pool::Get(handle);
    DVASSERT(buf);

    dx9_Check_Query_Results(buf);

    if (objectIndex < uint32(buf->results.size()))
    {
        return buf->results[objectIndex];
    }

    return 0;
}

namespace QueryBufferDX9
{
void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_QueryBuffer_Create = &dx9_QueryBuffer_Create;
    dispatch->impl_QueryBuffer_Reset = &dx9_QueryBuffer_Reset;
    dispatch->impl_QueryBuffer_Delete = &dx9_QueryBuffer_Delete;
    dispatch->impl_QueryBuffer_IsReady = &dx9_QueryBuffer_IsReady;
    dispatch->impl_QueryBuffer_ObjectIsReady = &dx9_QueryBuffer_ObjectIsReady;
    dispatch->impl_QueryBuffer_Value = &dx9_QueryBuffer_Value;
}

void SetQueryIndex(Handle handle, uint32 objectIndex)
{
    QueryBufferDX9_t* buf = QueryBufferDX9Pool::Get(handle);
    DVASSERT(buf);

    if (buf->curObjectIndex != objectIndex)
    {
        if (buf->curObjectIndex != DAVA::InvalidIndex)
        {
            buf->pendingQueries.back().first->Issue(D3DISSUE_END);
            buf->curObjectIndex = DAVA::InvalidIndex;
        }

        if (objectIndex != DAVA::InvalidIndex)
        {
            IDirect3DQuery9* iq = nullptr;
            if (QueryDX9Pool.size())
            {
                iq = QueryDX9Pool.back();
                QueryDX9Pool.pop_back();
            }
            else
            {
                _D3D9_Device->CreateQuery(D3DQUERYTYPE_OCCLUSION, &iq);
            }

            if (iq)
            {
                iq->Issue(D3DISSUE_BEGIN);
                buf->pendingQueries.push_back(std::make_pair(iq, objectIndex));

                buf->curObjectIndex = objectIndex;
            }
        }
    }
}

void QueryComplete(Handle handle)
{
    QueryBufferDX9_t* buf = QueryBufferDX9Pool::Get(handle);
    DVASSERT(buf);

    if (buf->curObjectIndex != DAVA::InvalidIndex)
    {
        buf->pendingQueries.back().first->Issue(D3DISSUE_END);
        buf->curObjectIndex = DAVA::InvalidIndex;
    }

    buf->bufferCompleted = true;
}

bool QueryIsCompleted(Handle handle)
{
    QueryBufferDX9_t* buf = QueryBufferDX9Pool::Get(handle);
    DVASSERT(buf);

    return buf->bufferCompleted;
}

void ReleaseQueryPool()
{
    std::vector<DX9Command> commands;
    commands.reserve(QueryDX9Pool.size());
    for (size_t i = 0; i < QueryDX9Pool.size(); ++i)
    {
        commands.push_back({ DX9Command::RELEASE, { reinterpret_cast<uint64>(QueryDX9Pool.data() + i) } });
    }
    ExecDX9(commands.data(), uint32(commands.size()), false);
    QueryDX9Pool.clear();
}

void ReleaseAll()
{
    for (QueryBufferDX9_t* buf : queryBuffers)
    {
        memset(buf->results.data(), 0, sizeof(uint32) * buf->results.size());

        if (buf->pendingQueries.size())
        {
            for (size_t q = 0; q < buf->pendingQueries.size(); ++q)
                QueryDX9Pool.push_back(buf->pendingQueries[q].first);

            buf->pendingQueries.clear();
        }

        buf->curObjectIndex = DAVA::InvalidIndex;
        buf->bufferCompleted = true;
    }

    for (IDirect3DQuery9* iq : QueryDX9Pool)
        iq->Release();

    QueryDX9Pool.clear();
}
}

//==============================================================================
} // namespace rhi
