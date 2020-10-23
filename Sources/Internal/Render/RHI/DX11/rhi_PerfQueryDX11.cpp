#include "rhi_DX11.h"

namespace rhi
{
struct PerfQueryDX11_t
{
    ID3D11Query* query = nullptr;
    uint64 timestamp = 0;
    uint64 freq = 0;
    uint32 isUsed : 1;
    uint32 isReady : 1;
    uint32 isFreqValid : 1;
};
using PerfQueryDX11Pool = ResourcePool<PerfQueryDX11_t, RESOURCE_PERFQUERY, PerfQuery::Descriptor, false>;
RHI_IMPL_POOL(PerfQueryDX11_t, RESOURCE_PERFQUERY, PerfQuery::Descriptor, false);

struct PerfQueryFrameDX11
{
    ID3D11Query* freqQuery = nullptr;
    DAVA::List<HPerfQuery> perfQueries;
    uint64 freq = 0;
    bool isFreqValid = false;
};

namespace PerfQueryDX11
{
PerfQueryFrameDX11* NextPerfQueryFrame();
DAVA::List<PerfQueryFrameDX11*> pendingPerfQueryFrameDX11;
DAVA::Vector<PerfQueryFrameDX11*> perfQueryFramePoolDX11;
PerfQueryFrameDX11* currentPerfQueryFrameDX11 = nullptr;
}

//==============================================================================

static Handle dx11_PerfQuery_Create()
{
    Handle handle = PerfQueryDX11Pool::Alloc();
    PerfQueryDX11_t* perfQuery = PerfQueryDX11Pool::Get(handle);

    if (perfQuery)
    {
        perfQuery->timestamp = 0;
        perfQuery->freq = 0;
        perfQuery->isUsed = 0;
        perfQuery->isReady = 0;
        perfQuery->isFreqValid = 0;

        DVASSERT(perfQuery->query == nullptr);

        D3D11_QUERY_DESC desc = { D3D11_QUERY_TIMESTAMP };
        if (!DX11DeviceCommand(DX11Command::CREATE_QUERY, &desc, &perfQuery->query))
        {
            PerfQueryDX11Pool::Free(handle);
            handle = InvalidHandle;
        }
    }

    return handle;
}

static void dx11_PerfQuery_Delete(Handle handle)
{
    PerfQueryDX11_t* perfQuery = PerfQueryDX11Pool::Get(handle);

    if (perfQuery)
    {
        perfQuery->query->Release();
        perfQuery->query = nullptr;
    }

    PerfQueryDX11Pool::Free(handle);
}

static void dx11_PerfQuery_Reset(Handle handle)
{
    PerfQueryDX11_t* perfQuery = PerfQueryDX11Pool::Get(handle);

    if (perfQuery)
    {
        perfQuery->freq = 0;
        perfQuery->isFreqValid = 0;
        perfQuery->isReady = 0;
        perfQuery->isUsed = 0;
        perfQuery->timestamp = 0;
    }
}

static bool dx11_PerfQuery_IsReady(Handle handle)
{
    bool ret = false;
    PerfQueryDX11_t* perfQuery = PerfQueryDX11Pool::Get(handle);

    if (perfQuery)
    {
        ret = perfQuery->isReady;
    }

    return ret;
}

static uint64 dx11_PerfQuery_Value(Handle handle)
{
    uint64 ret = 0;
    PerfQueryDX11_t* perfQuery = PerfQueryDX11Pool::Get(handle);
    if (perfQuery && perfQuery->isReady)
    {
        if (perfQuery->isFreqValid)
            ret = perfQuery->timestamp / (perfQuery->freq / 1000000); //mcs
        else
            ret = uint64(-1);
    }

    return ret;
}

namespace PerfQueryDX11
{
void IssueTimestampQuery(Handle handle, ID3D11DeviceContext* context)
{
    DVASSERT(currentPerfQueryFrameDX11);

    PerfQueryDX11_t* perfQuery = PerfQueryDX11Pool::Get(handle);
    if (perfQuery)
    {
        DVASSERT(perfQuery->query);
        DVASSERT(!perfQuery->isUsed);

        context->End(perfQuery->query);

        perfQuery->isUsed = 1;
        perfQuery->isReady = 0;

        currentPerfQueryFrameDX11->perfQueries.push_back(HPerfQuery(handle));
    }
}

void BeginMeasurment(ID3D11DeviceContext* context)
{
    DVASSERT(currentPerfQueryFrameDX11 == nullptr || !DeviceCaps().isPerfQuerySupported);

    currentPerfQueryFrameDX11 = NextPerfQueryFrame();

    if (currentPerfQueryFrameDX11)
        context->Begin(currentPerfQueryFrameDX11->freqQuery);
}

void EndMeasurment(ID3D11DeviceContext* context)
{
    DVASSERT(currentPerfQueryFrameDX11 || !DeviceCaps().isPerfQuerySupported);

    if (currentPerfQueryFrameDX11)
    {
        context->End(currentPerfQueryFrameDX11->freqQuery);

        pendingPerfQueryFrameDX11.push_back(currentPerfQueryFrameDX11);
        currentPerfQueryFrameDX11 = nullptr;
    }
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_PerfQuery_Create = &dx11_PerfQuery_Create;
    dispatch->impl_PerfQuery_Delete = &dx11_PerfQuery_Delete;
    dispatch->impl_PerfQuery_Reset = &dx11_PerfQuery_Reset;
    dispatch->impl_PerfQuery_IsReady = &dx11_PerfQuery_IsReady;
    dispatch->impl_PerfQuery_Value = &dx11_PerfQuery_Value;
}

//==============================================================================

void DeferredPerfQueriesIssued(const std::vector<Handle>& queries)
{
    if (!queries.empty())
    {
        DVASSERT(currentPerfQueryFrameDX11);
        currentPerfQueryFrameDX11->perfQueries.insert(currentPerfQueryFrameDX11->perfQueries.end(), queries.begin(), queries.end());
    }
}

//==============================================================================

void IssueTimestampQueryDeferred(Handle handle, ID3D11DeviceContext* context)
{
    PerfQueryDX11_t* perfQuery = PerfQueryDX11Pool::Get(handle);
    if (perfQuery)
    {
        DVASSERT(perfQuery->query);
        DVASSERT(!perfQuery->isUsed);

        context->End(perfQuery->query);

        perfQuery->isUsed = 1;
        perfQuery->isReady = 0;
    }
}

//==============================================================================

PerfQueryFrameDX11* NextPerfQueryFrame()
{
    PerfQueryFrameDX11* ret = nullptr;

    if (DeviceCaps().isPerfQuerySupported)
    {
        if (perfQueryFramePoolDX11.size())
        {
            ret = perfQueryFramePoolDX11.back();
            perfQueryFramePoolDX11.pop_back();
        }

        if (ret)
        {
            ret->freq = 0;
            ret->isFreqValid = false;
            ret->perfQueries.clear();
        }
        else
        {
            ret = new PerfQueryFrameDX11();
            D3D11_QUERY_DESC desc = { D3D11_QUERY_TIMESTAMP_DISJOINT };
            DX11DeviceCommand(DX11Command::CREATE_QUERY, &desc, &ret->freqQuery);
            DVASSERT(ret->freqQuery != nullptr);
        }
    }

    return ret;
}

//==============================================================================

void ObtainPerfQueryMeasurment(ID3D11DeviceContext* context)
{
    DAVA::List<PerfQueryFrameDX11*>::iterator fit = pendingPerfQueryFrameDX11.begin();
    while (fit != pendingPerfQueryFrameDX11.end())
    {
        PerfQueryFrameDX11* frame = *fit;

        if (frame->freq == 0)
        {
            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data = {};
            if (DX11Check(context->GetData(frame->freqQuery, &data, sizeof(data), 0)))
            {
                frame->isFreqValid = !data.Disjoint;
                frame->freq = data.Frequency;
            }
        }

        DAVA::List<HPerfQuery>::iterator qit = frame->perfQueries.begin();
        while (qit != frame->perfQueries.end())
        {
            PerfQueryDX11_t* query = PerfQueryDX11Pool::Get(*qit);

            DVASSERT(query->isUsed);

            if (query->timestamp == 0)
            {
                DX11Check(context->GetData(query->query, &(query->timestamp), sizeof(uint64), 0));
            }

            if (frame->freq && query->timestamp)
            {
                query->freq = frame->freq;
                query->isFreqValid = frame->isFreqValid;
                query->isReady = 1;

                qit = frame->perfQueries.erase(qit);
            }
            else
            {
                ++qit;
            }
        }

        ++fit;
    }

    fit = pendingPerfQueryFrameDX11.begin();
    while (fit != pendingPerfQueryFrameDX11.end())
    {
        PerfQueryFrameDX11* frame = *fit;

        if (frame->freq && !frame->perfQueries.size())
        {
            perfQueryFramePoolDX11.push_back(frame);
            fit = pendingPerfQueryFrameDX11.erase(fit);
        }
        else
        {
            ++fit;
        }
    }
}

void ReleasePerfQueryPool()
{
    for (PerfQueryFrameDX11* frame : pendingPerfQueryFrameDX11)
    {
        frame->freqQuery->Release();
        delete frame;
    }
    pendingPerfQueryFrameDX11.clear();

    for (PerfQueryFrameDX11* frame : perfQueryFramePoolDX11)
    {
        frame->freqQuery->Release();
        delete frame;
    }
    perfQueryFramePoolDX11.clear();
}
}

//==============================================================================
} // namespace rhi
