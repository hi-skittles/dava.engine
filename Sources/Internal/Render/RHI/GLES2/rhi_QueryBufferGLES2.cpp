#include "../Common/rhi_Private.h"
    #include "../Common/rhi_Pool.h"
    #include "rhi_GLES2.h"

    #include "Debug/DVAssert.h"
    #include "Logger/Logger.h"
using DAVA::Logger;

    #include "_gl.h"

#if defined(__DAVAENGINE_IPHONE__)

#define _glBeginQuery(q) glBeginQueryEXT(GL_ANY_SAMPLES_PASSED_EXT, q)
#define _glEndQuery() glEndQueryEXT(GL_ANY_SAMPLES_PASSED_EXT)

#elif defined(__DAVAENGINE_ANDROID__)

#define _glBeginQuery(q)
#define _glEndQuery()

#elif defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_WIN32__)

#define _glBeginQuery(q) glBeginQuery(GL_SAMPLES_PASSED, q)
#define _glEndQuery() glEndQuery(GL_SAMPLES_PASSED)

#else

#define _glBeginQuery(q) glBeginQuery(GL_ANY_SAMPLES_PASSED, q)
#define _glEndQuery() glEndQuery(GL_ANY_SAMPLES_PASSED)

#endif

namespace rhi
{
//==============================================================================

class
QueryBufferGLES2_t
{
public:
    QueryBufferGLES2_t()
        : curObjectIndex(DAVA::InvalidIndex)
        , bufferCompleted(false){};
    ~QueryBufferGLES2_t(){};

    std::vector<std::pair<GLuint, uint32>> pendingQueries;
    std::vector<uint32> results;
    uint32 curObjectIndex;
    uint32 bufferCompleted : 1;
};

typedef ResourcePool<QueryBufferGLES2_t, RESOURCE_QUERY_BUFFER, QueryBuffer::Descriptor, false> QueryBufferGLES2Pool;
RHI_IMPL_POOL(QueryBufferGLES2_t, RESOURCE_QUERY_BUFFER, QueryBuffer::Descriptor, false);

std::vector<GLuint> QueryObjectGLES2Pool;

//==============================================================================

static Handle gles2_QueryBuffer_Create(uint32 maxObjectCount)
{
    Handle handle = QueryBufferGLES2Pool::Alloc();
    QueryBufferGLES2_t* buf = QueryBufferGLES2Pool::Get(handle);
    DVASSERT(buf);

    buf->results.resize(maxObjectCount);
    memset(buf->results.data(), 0, sizeof(uint32) * buf->results.size());
    buf->pendingQueries.clear();
    buf->curObjectIndex = DAVA::InvalidIndex;
    buf->bufferCompleted = false;

    return handle;
}

static void gles2_QueryBuffer_Reset(Handle handle)
{
    QueryBufferGLES2_t* buf = QueryBufferGLES2Pool::Get(handle);
    DVASSERT(buf);

    memset(buf->results.data(), 0, sizeof(uint32) * buf->results.size());

    if (buf->pendingQueries.size())
    {
        for (size_t q = 0; q < buf->pendingQueries.size(); ++q)
            QueryObjectGLES2Pool.push_back(buf->pendingQueries[q].first);

        buf->pendingQueries.clear();
    }

    buf->curObjectIndex = DAVA::InvalidIndex;
    buf->bufferCompleted = false;
}

static void gles2_QueryBuffer_Delete(Handle handle)
{
    QueryBufferGLES2_t* buf = QueryBufferGLES2Pool::Get(handle);
    DVASSERT(buf);

    if (buf->pendingQueries.size())
    {
        for (size_t q = 0; q < buf->pendingQueries.size(); ++q)
            QueryObjectGLES2Pool.push_back(buf->pendingQueries[q].first);

        buf->pendingQueries.clear();
    }

    QueryBufferGLES2Pool::Free(handle);
}

static void gles2_Check_Query_Results(QueryBufferGLES2_t* buf)
{
    if (buf->pendingQueries.empty())
        return;

    const uint32 GLES2_MAX_PENDING_QUERIES = 256;

    GLCommand smallCmdBuffer[GLES2_MAX_PENDING_QUERIES];
    static DAVA::Vector<GLCommand> largeCmdBuffer;

    uint32 smallResultsBuffer[GLES2_MAX_PENDING_QUERIES];
    static DAVA::Vector<uint32> largeResultsBuffer;

    uint32 cmdCount = uint32(buf->pendingQueries.size());

    GLCommand* cmd = smallCmdBuffer;
    uint32* results = smallResultsBuffer;
    if (cmdCount > GLES2_MAX_PENDING_QUERIES)
    {
        largeCmdBuffer.resize(cmdCount);
        cmd = largeCmdBuffer.data();

        largeResultsBuffer.resize(cmdCount);
        results = largeResultsBuffer.data();
    }

    for (uint32 q = 0; q < cmdCount; ++q)
    {
        results[q] = uint32(-1);
        cmd[q] = { GLCommand::GET_QUERY_RESULT_NO_WAIT, { uint64(buf->pendingQueries[q].first), uint64(&results[q]) } };
    }

    ExecGL(cmd, cmdCount);

    for (int32 q = cmdCount - 1; q >= 0; --q)
    {
        uint32 resultIndex = buf->pendingQueries[q].second;
        if (results[q] != uint32(-1))
        {
            if (resultIndex < buf->results.size())
                buf->results[resultIndex] += results[q];

            QueryObjectGLES2Pool.push_back(buf->pendingQueries[q].first);

            buf->pendingQueries[q] = buf->pendingQueries.back();
            buf->pendingQueries.pop_back();
        }
    }
}

static bool gles2_QueryBuffer_IsReady(Handle handle)
{
    bool ready = false;
    QueryBufferGLES2_t* buf = QueryBufferGLES2Pool::Get(handle);
    DVASSERT(buf);

    if (buf->bufferCompleted)
    {
        gles2_Check_Query_Results(buf);
        ready = (buf->pendingQueries.size() == 0);
    }

    return ready;
}

static bool gles2_QueryBuffer_ObjectIsReady(Handle handle, uint32 objectIndex)
{
    bool ready = false;
    QueryBufferGLES2_t* buf = QueryBufferGLES2Pool::Get(handle);
    DVASSERT(buf);

    if (buf->bufferCompleted)
    {
        gles2_Check_Query_Results(buf);

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

static int32 gles2_QueryBuffer_Value(Handle handle, uint32 objectIndex)
{
    QueryBufferGLES2_t* buf = QueryBufferGLES2Pool::Get(handle);
    DVASSERT(buf);

    gles2_Check_Query_Results(buf);

    if (objectIndex < buf->results.size())
    {
        return buf->results[objectIndex];
    }

    return 0;
}

namespace QueryBufferGLES2
{
void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_QueryBuffer_Create = &gles2_QueryBuffer_Create;
    dispatch->impl_QueryBuffer_Reset = &gles2_QueryBuffer_Reset;
    dispatch->impl_QueryBuffer_Delete = &gles2_QueryBuffer_Delete;
    dispatch->impl_QueryBuffer_IsReady = &gles2_QueryBuffer_IsReady;
    dispatch->impl_QueryBuffer_ObjectIsReady = &gles2_QueryBuffer_ObjectIsReady;
    dispatch->impl_QueryBuffer_Value = &gles2_QueryBuffer_Value;
}

void SetQueryIndex(Handle handle, uint32 objectIndex)
{
    QueryBufferGLES2_t* buf = QueryBufferGLES2Pool::Get(handle);
    DVASSERT(buf);

    if (buf->curObjectIndex != objectIndex)
    {
        if (buf->curObjectIndex != DAVA::InvalidIndex)
        {
            _glEndQuery();
            buf->curObjectIndex = DAVA::InvalidIndex;
        }

        if (objectIndex != DAVA::InvalidIndex)
        {
            GLuint q = 0;
            if (QueryObjectGLES2Pool.size())
            {
                q = QueryObjectGLES2Pool.back();
                QueryObjectGLES2Pool.pop_back();
            }
            else
            {
#if defined(__DAVAENGINE_IPHONE__)
                glGenQueriesEXT(1, &q);
#elif defined(__DAVAENGINE_ANDROID__)
#else
                glGenQueries(1, &q);
#endif
            }

            if (q)
            {
                _glBeginQuery(q);
                buf->pendingQueries.push_back(std::make_pair(q, objectIndex));

                buf->curObjectIndex = objectIndex;
            }
        }
    }
}

void QueryComplete(Handle handle)
{
    QueryBufferGLES2_t* buf = QueryBufferGLES2Pool::Get(handle);
    DVASSERT(buf);

    if (buf->curObjectIndex != DAVA::InvalidIndex)
    {
        _glEndQuery();
        buf->curObjectIndex = DAVA::InvalidIndex;
    }

    buf->bufferCompleted = true;
}

bool QueryIsCompleted(Handle handle)
{
    QueryBufferGLES2_t* buf = QueryBufferGLES2Pool::Get(handle);
    DVASSERT(buf);

    return buf->bufferCompleted;
}

void ReleaseQueryObjectsPool()
{
    if (QueryObjectGLES2Pool.size())
    {
        GLCommand cmd = { GLCommand::DELETE_QUERIES, { uint64(QueryObjectGLES2Pool.size()), uint64(QueryObjectGLES2Pool.data()) } };
        ExecGL(&cmd, 1);

        QueryObjectGLES2Pool.clear();
    }
}
}

//==============================================================================
} // namespace rhi
