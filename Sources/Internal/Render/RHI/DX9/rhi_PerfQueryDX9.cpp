#include "../Common/rhi_Private.h"
    #include "../Common/rhi_Pool.h"
    #include "rhi_DX9.h"

    #include "Debug/DVAssert.h"
    #include "Logger/Logger.h"
using DAVA::Logger;

    #include "_dx9.h"

namespace rhi
{
//==============================================================================

class PerfQueryDX9_t
{
public:
    IDirect3DQuery9* query = nullptr;
    uint64 timestamp = 0;
    uint64 freq = 0;
    uint32 isUsed : 1;
    uint32 isReady : 1;
    uint32 isValid : 1;
};

namespace PerfQueryDX9
{
struct PerfQueryFrameDX9
{
    IDirect3DQuery9* disjointQuery = nullptr;
    IDirect3DQuery9* freqQuery = nullptr;
    DAVA::List<HPerfQuery> perfQueries;
    uint64 freq = 0;
    bool isValid = false;
};

PerfQueryFrameDX9* NextPerfQueryFrame();
}
//==============================================================================

typedef ResourcePool<PerfQueryDX9_t, RESOURCE_PERFQUERY, PerfQuery::Descriptor, false> PerfQueryDX9Pool;
RHI_IMPL_POOL(PerfQueryDX9_t, RESOURCE_PERFQUERY, PerfQuery::Descriptor, false);

DAVA::Mutex perfQueryFramePoolSyncDX9;
DAVA::List<PerfQueryDX9::PerfQueryFrameDX9*> pendingPerfQueryFrameDX9;
DAVA::Vector<PerfQueryDX9::PerfQueryFrameDX9*> perfQueryFramePoolDX9;
PerfQueryDX9::PerfQueryFrameDX9* currentPerfQueryFrameDX9 = nullptr;

//==============================================================================

static Handle dx9_PerfQuery_Create()
{
    Handle handle = PerfQueryDX9Pool::Alloc();
    PerfQueryDX9_t* perfQuery = PerfQueryDX9Pool::Get(handle);

    if (perfQuery)
    {
        perfQuery->timestamp = 0;
        perfQuery->freq = 0;
        perfQuery->isUsed = 0;
        perfQuery->isReady = 0;
        perfQuery->isValid = 0;

        DVASSERT(perfQuery->query == nullptr);
    }

    return handle;
}

static void dx9_PerfQuery_Delete(Handle handle)
{
    PerfQueryDX9_t* perfQuery = PerfQueryDX9Pool::Get(handle);

    if (perfQuery)
    {
        DX9Command cmd = { DX9Command::RELEASE, { uint64_t(&perfQuery->query) } };
        ExecDX9(&cmd, 1, true);
        perfQuery->query = nullptr;
    }

    PerfQueryDX9Pool::Free(handle);
}

static void dx9_PerfQuery_Reset(Handle handle)
{
    PerfQueryDX9_t* perfQuery = PerfQueryDX9Pool::Get(handle);

    if (perfQuery)
    {
        perfQuery->timestamp = 0;
        perfQuery->freq = 0;
        perfQuery->isValid = 0;
        perfQuery->isReady = 0;
        perfQuery->isUsed = 0;
    }
}

static bool dx9_PerfQuery_IsReady(Handle handle)
{
    bool ret = false;
    PerfQueryDX9_t* perfQuery = PerfQueryDX9Pool::Get(handle);

    if (perfQuery)
    {
        ret = perfQuery->isReady;
    }

    return ret;
}

static uint64 dx9_PerfQuery_Value(Handle handle)
{
    uint64 ret = 0;
    PerfQueryDX9_t* perfQuery = PerfQueryDX9Pool::Get(handle);
    if (perfQuery && perfQuery->isReady)
    {
        if (perfQuery->isValid)
            ret = perfQuery->timestamp / (perfQuery->freq / 1000000); //mcs
        else
            ret = uint64(-1);
    }

    return ret;
}

namespace PerfQueryDX9
{
void IssueTimestampQuery(Handle handle)
{
    DVASSERT(currentPerfQueryFrameDX9);

    PerfQueryDX9_t* perfQuery = PerfQueryDX9Pool::Get(handle);
    if (perfQuery)
    {
        DVASSERT(!perfQuery->isUsed);

        if (!perfQuery->query)
            _D3D9_Device->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &(perfQuery->query));

        perfQuery->query->Issue(D3DISSUE_END);

        perfQuery->isUsed = 1;
        perfQuery->isReady = 0;

        currentPerfQueryFrameDX9->perfQueries.push_back(HPerfQuery(handle));
    }
}

void BeginMeasurment()
{
    DVASSERT(currentPerfQueryFrameDX9 == nullptr);

    if (DeviceCaps().isPerfQuerySupported)
    {
        currentPerfQueryFrameDX9 = NextPerfQueryFrame();

        if (!currentPerfQueryFrameDX9->disjointQuery)
            _D3D9_Device->CreateQuery(D3DQUERYTYPE_TIMESTAMPDISJOINT, &(currentPerfQueryFrameDX9->disjointQuery));

        if (!currentPerfQueryFrameDX9->freqQuery)
            _D3D9_Device->CreateQuery(D3DQUERYTYPE_TIMESTAMPFREQ, &(currentPerfQueryFrameDX9->freqQuery));

        currentPerfQueryFrameDX9->disjointQuery->Issue(D3DISSUE_BEGIN);
        currentPerfQueryFrameDX9->freqQuery->Issue(D3DISSUE_END);
    }
}

void EndMeasurment()
{
    if (currentPerfQueryFrameDX9)
    {
        currentPerfQueryFrameDX9->disjointQuery->Issue(D3DISSUE_END);

        pendingPerfQueryFrameDX9.push_back(currentPerfQueryFrameDX9);
        currentPerfQueryFrameDX9 = nullptr;
    }
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_PerfQuery_Create = &dx9_PerfQuery_Create;
    dispatch->impl_PerfQuery_Delete = &dx9_PerfQuery_Delete;
    dispatch->impl_PerfQuery_Reset = &dx9_PerfQuery_Reset;
    dispatch->impl_PerfQuery_IsReady = &dx9_PerfQuery_IsReady;
    dispatch->impl_PerfQuery_Value = &dx9_PerfQuery_Value;
}

//==============================================================================

PerfQueryFrameDX9* NextPerfQueryFrame()
{
    currentPerfQueryFrameDX9 = nullptr;

    perfQueryFramePoolSyncDX9.Lock();
    if (perfQueryFramePoolDX9.size())
    {
        currentPerfQueryFrameDX9 = perfQueryFramePoolDX9.back();
        perfQueryFramePoolDX9.pop_back();
    }
    perfQueryFramePoolSyncDX9.Unlock();

    if (currentPerfQueryFrameDX9)
    {
        currentPerfQueryFrameDX9->freq = 0;
        currentPerfQueryFrameDX9->isValid = false;
        currentPerfQueryFrameDX9->perfQueries.clear();
    }
    else
    {
        currentPerfQueryFrameDX9 = new PerfQueryFrameDX9();
    }

    return currentPerfQueryFrameDX9;
}

//==============================================================================

void ReleaseAll()
{
    PerfQueryDX9Pool::Iterator it = PerfQueryDX9Pool::Begin(), end = PerfQueryDX9Pool::End();
    for (; it != end; ++it)
    {
        if (it->query)
        {
            Logger::Error("ReleaseQuery %x", it->query);
            it->query->Release();
            it->query = nullptr;
        }
    }

    if (currentPerfQueryFrameDX9)
    {
        if (currentPerfQueryFrameDX9->disjointQuery)
            currentPerfQueryFrameDX9->disjointQuery->Release();
        if (currentPerfQueryFrameDX9->freqQuery)
            currentPerfQueryFrameDX9->freqQuery->Release();

        currentPerfQueryFrameDX9->disjointQuery = nullptr;
        currentPerfQueryFrameDX9->freqQuery = nullptr;
    }

    DAVA::LockGuard<DAVA::Mutex> guard(perfQueryFramePoolSyncDX9);

    perfQueryFramePoolDX9.insert(perfQueryFramePoolDX9.end(), pendingPerfQueryFrameDX9.begin(), pendingPerfQueryFrameDX9.end());
    pendingPerfQueryFrameDX9.clear();

    for (PerfQueryFrameDX9* frame : perfQueryFramePoolDX9)
    {
        if (frame->disjointQuery)
            frame->disjointQuery->Release();

        if (frame->freqQuery)
            frame->freqQuery->Release();
        delete frame;
    }
    perfQueryFramePoolDX9.clear();
}

void ObtainPerfQueryMeasurment()
{
    for (PerfQueryFrameDX9* frame : pendingPerfQueryFrameDX9)
    {
        if (!frame->freq)
        {
            bool disjoint = false;
            uint64 frequency;

            HRESULT hr = frame->disjointQuery->GetData(&disjoint, sizeof(bool), 0);
            if (hr == S_OK)
            {
                frame->isValid = !disjoint;
            }

            hr = frame->freqQuery->GetData(&frequency, sizeof(uint64), 0);
            if (hr == S_OK)
            {
                frame->freq = frequency;
            }
        }

        DAVA::List<HPerfQuery>::iterator qit = frame->perfQueries.begin();
        while (qit != frame->perfQueries.end())
        {
            PerfQueryDX9_t* query = PerfQueryDX9Pool::Get(*qit);

            DVASSERT(query->isUsed);

            if (!query->timestamp)
            {
                uint64 ts = 0;
                HRESULT hr = query->query->GetData(&ts, sizeof(uint64), 0);
                if (hr == S_OK)
                {
                    query->timestamp = ts;
                }
            }

            if (frame->freq && query->timestamp)
            {
                query->freq = frame->freq;
                query->isValid = frame->isValid;
                query->isReady = 1;

                qit = frame->perfQueries.erase(qit);
            }
            else
            {
                ++qit;
            }
        }
    }

    DAVA::LockGuard<DAVA::Mutex> guard(perfQueryFramePoolSyncDX9);

    DAVA::List<PerfQueryFrameDX9*>::iterator fit = pendingPerfQueryFrameDX9.begin();
    while (fit != pendingPerfQueryFrameDX9.end())
    {
        PerfQueryFrameDX9* frame = *fit;

        if (frame->freq && !frame->perfQueries.size())
        {
            perfQueryFramePoolDX9.push_back(frame);
            fit = pendingPerfQueryFrameDX9.erase(fit);
        }
        else
        {
            ++fit;
        }
    }
}

void ReleasePerfQueryPool()
{
    std::vector<DX9Command> cmd;
    for (PerfQueryFrameDX9* frame : pendingPerfQueryFrameDX9)
    {
        cmd.push_back({ DX9Command::RELEASE, { uint64(&frame->disjointQuery) } });
        cmd.push_back({ DX9Command::RELEASE, { uint64(&frame->freqQuery) } });
    }

    for (PerfQueryFrameDX9* frame : perfQueryFramePoolDX9)
    {
        cmd.push_back({ DX9Command::RELEASE, { uint64(&frame->disjointQuery) } });
        cmd.push_back({ DX9Command::RELEASE, { uint64(&frame->freqQuery) } });
    }

    ExecDX9(cmd.data(), uint32(cmd.size()), false);

    for (PerfQueryFrameDX9* frame : pendingPerfQueryFrameDX9)
    {
        delete frame;
    }
    pendingPerfQueryFrameDX9.clear();

    for (PerfQueryFrameDX9* frame : perfQueryFramePoolDX9)
    {
        delete frame;
    }
    perfQueryFramePoolDX9.clear();
}
}

//==============================================================================
} // namespace rhi
