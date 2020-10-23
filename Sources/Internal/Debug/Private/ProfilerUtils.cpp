#include "Debug/ProfilerUtils.h"
#include "Debug/TraceEvent.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerGPU.h"
#include "Base/BaseTypes.h"
#include "Debug/DVAssert.h"
#include "FileSystem/FileSystem.h"
#include "Render/RHI/rhi_Public.h"
#include "Base/FastName.h"
#include <sstream>

namespace DAVA
{
namespace ProfilerUtils
{
void DumpCPUGPUTrace(ProfilerCPU* cpuProfiler, ProfilerGPU* gpuProfiler, std::ostream& stream)
{
    DVASSERT(!cpuProfiler->IsStarted() && !gpuProfiler->IsStarted());

    uint64 cpuTime = 0, gpuTime = 0;
    rhi::SynchronizeCPUGPU(&cpuTime, &gpuTime);

    uint64 minTime = Min(cpuTime, gpuTime);
    cpuTime -= minTime;
    gpuTime -= minTime;

    Vector<TraceEvent> cpuTrace = cpuProfiler->GetTrace();
    Vector<TraceEvent> gpuTrace = gpuProfiler->GetTrace();

    if (cpuTime)
    {
        for (TraceEvent& trace : cpuTrace)
            trace.timestamp -= cpuTime;
    }

    if (gpuTime)
    {
        for (TraceEvent& trace : gpuTrace)
            trace.timestamp -= gpuTime;
    }

    gpuTrace.insert(gpuTrace.end(), cpuTrace.begin(), cpuTrace.end());

    TraceEvent::DumpJSON(gpuTrace, stream);
}

void DumpCPUGPUTraceToFile(ProfilerCPU* cpuProfiler, ProfilerGPU* gpuProfiler, const FilePath& filePath)
{
    DAVA::FileSystem::Instance()->DeleteFile(filePath);
    DAVA::File* json = DAVA::File::Create(filePath, DAVA::File::CREATE | DAVA::File::WRITE);
    if (json)
    {
        std::stringstream stream;
        DumpCPUGPUTrace(cpuProfiler, gpuProfiler, stream);
        json->WriteNonTerminatedString(stream.str());
    }
    SafeRelease(json);
}

}; //ns ProfilerDump
}; //ns DAVA
