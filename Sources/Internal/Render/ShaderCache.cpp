#include "Render/ShaderCache.h"
#include "Render/RHI/rhi_ShaderCache.h"
#include "FileSystem/FileSystem.h"
#include "Concurrency/LockGuard.h"
#include "Logger/Logger.h"
#include "Utils/StringFormat.h"
#include "Render/RHI/rhi_ShaderSource.h"

#define RHI_TRACE_CACHE_USAGE 0

namespace DAVA
{
namespace ShaderDescriptorCache
{
struct ShaderSourceCode
{
    Vector<char> vertexProgText;
    Vector<char> fragmentProgText;

    FilePath vertexProgSourcePath;
    FilePath fragmentProgSourcePath;

    uint32 vSrcHash = 0;
    uint32 fSrcHash = 0;
};

namespace
{
Map<Vector<size_t>, ShaderDescriptor*> shaderDescriptors;
Map<FastName, ShaderSourceCode> shaderSourceCodes;
Mutex shaderCacheMutex;
bool loadingNotifyEnabled = false;
bool initialized = false;
}

void Initialize()
{
    DVASSERT(!initialized);
    initialized = true;
}

void Uninitialize()
{
    DVASSERT(initialized);
    Clear();
    initialized = false;
}

void Clear()
{
    DVASSERT(initialized);
    LockGuard<Mutex> guard(shaderCacheMutex);
    shaderSourceCodes.clear();
}

void ClearDynamicBindigs()
{
    DVASSERT(initialized);

    LockGuard<Mutex> guard(shaderCacheMutex);

    for (auto& it : shaderDescriptors)
    {
        it.second->ClearDynamicBindings();
    }
}

size_t GetUniqueFlagKey(FastName flagName)
{
    static_assert(sizeof(size_t) == sizeof(void*), "Cant cast `const char*` into `int`");
    return reinterpret_cast<size_t>(flagName.c_str());
}

Vector<size_t> BuildFlagsKey(const FastName& name, const UnorderedMap<FastName, int32>& defines)
{
    Vector<size_t> key;

    key.reserve(defines.size() * 2 + 1);
    for (const auto& define : defines)
    {
        key.emplace_back(GetUniqueFlagKey(define.first));
        key.emplace_back(define.second);
    }

    // reinterpret cast to pairs and sort them
    using SizeTPair = std::pair<size_t, size_t>;
    SizeTPair* begin = reinterpret_cast<SizeTPair*>(key.data());
    std::sort(begin, begin + key.size() / 2, [](const SizeTPair& l, const SizeTPair& r) {
        return l.first < r.first;
    });

    key.push_back(GetUniqueFlagKey(name));
    return key;
}

void LoadFromSource(const String& source, ShaderSourceCode& sourceCode)
{
    sourceCode.vertexProgSourcePath = FilePath(source + "-vp.sl");
    sourceCode.fragmentProgSourcePath = FilePath(source + "-fp.sl");

    //later move it into FileSystem

    //vertex
    bool loaded = true;
    File* fp = File::Create(sourceCode.vertexProgSourcePath, File::OPEN | File::READ);
    if (fp)
    {
        uint32 fileSize = static_cast<uint32>(fp->GetSize());
        sourceCode.vertexProgText.resize(fileSize + 1);
        sourceCode.vertexProgText[fileSize] = 0;
        uint32 dataRead = fp->Read(sourceCode.vertexProgText.data(), fileSize);
        if (dataRead != fileSize)
        {
            loaded = false;
            sourceCode.vertexProgText.resize(1);
            sourceCode.vertexProgText.shrink_to_fit();
            Logger::Error("Failed to open vertex shader source file: %s", sourceCode.vertexProgSourcePath.GetAbsolutePathname().c_str());
        }
    }
    else
    {
        loaded = false;
        Logger::Error("Failed to open vertex shader source file: %s", sourceCode.vertexProgSourcePath.GetAbsolutePathname().c_str());
    }
    SafeRelease(fp);

    if (!loaded)
    {
        sourceCode.vertexProgText.resize(1);
        sourceCode.vertexProgText[0] = 0;
    }

    //fragment
    loaded = true;
    fp = File::Create(sourceCode.fragmentProgSourcePath, File::OPEN | File::READ);
    if (fp)
    {
        uint32 fileSize = static_cast<uint32>(fp->GetSize());
        sourceCode.fragmentProgText.resize(fileSize + 1);
        sourceCode.fragmentProgText[fileSize] = 0;
        uint32 dataRead = fp->Read(sourceCode.fragmentProgText.data(), fileSize);
        if (dataRead != fileSize)
        {
            loaded = false;
            sourceCode.fragmentProgText.resize(1);
            sourceCode.fragmentProgText.shrink_to_fit();
            Logger::Error("Failed to open fragment shader source file: %s", sourceCode.fragmentProgSourcePath.GetAbsolutePathname().c_str());
        }
    }
    else
    {
        loaded = false;
        Logger::Error("Failed to open fragment shader source file: %s", sourceCode.fragmentProgSourcePath.GetAbsolutePathname().c_str());
    }
    SafeRelease(fp);

    if (!loaded)
    {
        sourceCode.fragmentProgText.resize(1);
        sourceCode.fragmentProgText[0] = 0;
    }

    sourceCode.vSrcHash = HashValue_N(sourceCode.vertexProgText.data(), static_cast<uint32>(strlen(sourceCode.vertexProgText.data())));
    sourceCode.fSrcHash = HashValue_N(sourceCode.fragmentProgText.data(), static_cast<uint32>(strlen(sourceCode.fragmentProgText.data())));
}

const ShaderSourceCode& GetSourceCode(const FastName& name)
{
    auto sourceIt = shaderSourceCodes.find(name);
    if (sourceIt != shaderSourceCodes.end()) //source found
        return sourceIt->second;

    LoadFromSource(name.c_str(), shaderSourceCodes[name]);
    return shaderSourceCodes.at(name);
}

void SetLoadingNotifyEnabled(bool enable)
{
    loadingNotifyEnabled = enable;
}


#define DUMP_SOURCES 0
#define TRACE_CACHE_USAGE 0

#if TRACE_CACHE_USAGE
#define LOG_TRACE_USAGE(usage, ...) Logger::Info(usage, __VA_ARGS__)
#else
#define LOG_TRACE_USAGE(...)
#endif

ShaderDescriptor* GetShaderDescriptor(const FastName& name, const UnorderedMap<FastName, int32>& defines)
{
    DVASSERT(initialized);

    LockGuard<Mutex> guard(shaderCacheMutex);

    Vector<size_t> key = BuildFlagsKey(name, defines);

    auto descriptorIt = shaderDescriptors.find(key);
    if (descriptorIt != shaderDescriptors.end())
        return descriptorIt->second;

    //not found - create new shader
    Vector<String> progDefines;
    progDefines.reserve(defines.size() * 2);
    String resName(name.c_str());
    resName += "  defines: ";
    for (auto& it : defines)
    {
        bool doAdd = true;

        for (size_t i = 0; i != progDefines.size(); i += 2)
        {
            if (strcmp(it.first.c_str(), progDefines[i].c_str()) < 0)
            {
                progDefines.insert(progDefines.begin() + i, String(it.first.c_str()));
                progDefines.insert(progDefines.begin() + i + 1, Format("%d", it.second));
                doAdd = false;
                break;
            }
        }

        if (doAdd)
        {
            progDefines.push_back(String(it.first.c_str()));
            progDefines.push_back(Format("%d", it.second));
        }
    }

    for (size_t i = 0; i != progDefines.size(); i += 2)
        resName += Format("%s = %s, ", progDefines[i + 0].c_str(), progDefines[i + 1].c_str());

    if (loadingNotifyEnabled)
    {
        Logger::Error("Forbidden call to GetShaderDescriptor %s", resName.c_str());
    }

    FastName vProgUid, fProgUid;
    vProgUid = FastName(String("vSource: ") + resName);
    fProgUid = FastName(String("fSource: ") + resName);

    ShaderSourceCode sourceCode = GetSourceCode(name);
    const uint32 vSrcHash = HashValue_N(sourceCode.vertexProgText.data(), static_cast<uint32>(strlen(sourceCode.vertexProgText.data())));
    const uint32 fSrcHash = HashValue_N(sourceCode.fragmentProgText.data(), static_cast<uint32>(strlen(sourceCode.fragmentProgText.data())));

    const rhi::ShaderSource* vSource = rhi::ShaderSourceCache::Get(vProgUid, vSrcHash);
    const rhi::ShaderSource* fSource = rhi::ShaderSourceCache::Get(fProgUid, fSrcHash);

    bool isCachedShader = false;

    if (!vSource || !fSource)
    {
        LOG_TRACE_USAGE("building \"%s\"", vProgUid.c_str());
        vSource = rhi::ShaderSourceCache::Add(sourceCode.vertexProgSourcePath.GetFrameworkPath().c_str(), vProgUid, rhi::PROG_VERTEX, sourceCode.vertexProgText.data(), progDefines);
        fSource = rhi::ShaderSourceCache::Add(sourceCode.fragmentProgSourcePath.GetFrameworkPath().c_str(), fProgUid, rhi::PROG_FRAGMENT, sourceCode.fragmentProgText.data(), progDefines);
    }
    else
    {
        LOG_TRACE_USAGE("using cached \"%s\"", vProgUid.c_str());
        isCachedShader = true;
    }

    if (!vSource || !fSource)
    {
        if (!vSource)
            Logger::Error("failed to construct vSource for \"%s\"", vProgUid.c_str());
        if (!fSource)
            Logger::Error("failed to construct fSource for \"%s\"", fProgUid.c_str());

        // don't try to create pipeline-state, return 'not-valid'
        rhi::PipelineState::Descriptor psDesc;
        psDesc.vprogUid = vProgUid;
        psDesc.fprogUid = fProgUid;
        ShaderDescriptor* res = new ShaderDescriptor(rhi::HPipelineState(rhi::InvalidHandle), vProgUid, fProgUid);
        res->sourceName = name;
        res->defines = defines;
        res->valid = false;
        shaderDescriptors[key] = res;
        return res;
    }

#if DUMP_SOURCES
    Logger::Info("\n\n%s", vProgUid.c_str());
    vSource->Dump();
    Logger::Info("\n\n%s", fProgUid.c_str());
    fSource->Dump();
#endif

    const std::string& vpBin = vSource->GetSourceCode(rhi::HostApi());
    rhi::ShaderCache::UpdateProgBinary(rhi::HostApi(), rhi::PROG_VERTEX, vProgUid, vpBin.c_str(), unsigned(vpBin.length()));
    const std::string& fpBin = fSource->GetSourceCode(rhi::HostApi());
    rhi::ShaderCache::UpdateProgBinary(rhi::HostApi(), rhi::PROG_FRAGMENT, fProgUid, fpBin.c_str(), unsigned(fpBin.length()));
    //ShaderDescr
    rhi::PipelineState::Descriptor psDesc;
    psDesc.vprogUid = vProgUid;
    psDesc.fprogUid = fProgUid;
    psDesc.vertexLayout = vSource->ShaderVertexLayout();
    psDesc.blending = fSource->Blending();
    rhi::HPipelineState piplineState = rhi::AcquireRenderPipelineState(psDesc);

    //in case we have broken shaders in cache, replace them with newly compiled
    if ((!piplineState.IsValid()) && isCachedShader)
    {
        DAVA::Logger::Info("cached shader compilation failed ");
        DAVA::Logger::Info("  vprog-uid = %s", vProgUid.c_str());
        DAVA::Logger::Info("  fprog-uid = %s", fProgUid.c_str());
        DAVA::Logger::Info("trying to replace from source files");

        vSource = rhi::ShaderSourceCache::Add(sourceCode.vertexProgSourcePath.GetFrameworkPath().c_str(), vProgUid, rhi::PROG_VERTEX, sourceCode.vertexProgText.data(), progDefines);
        fSource = rhi::ShaderSourceCache::Add(sourceCode.fragmentProgSourcePath.GetFrameworkPath().c_str(), fProgUid, rhi::PROG_FRAGMENT, sourceCode.fragmentProgText.data(), progDefines);

        const std::string& vpBin = vSource->GetSourceCode(rhi::HostApi());
        rhi::ShaderCache::UpdateProgBinary(rhi::HostApi(), rhi::PROG_VERTEX, vProgUid, vpBin.c_str(), unsigned(vpBin.length()));
        const std::string& fpBin = fSource->GetSourceCode(rhi::HostApi());
        rhi::ShaderCache::UpdateProgBinary(rhi::HostApi(), rhi::PROG_FRAGMENT, fProgUid, fpBin.c_str(), unsigned(fpBin.length()));

        psDesc.vprogUid = vProgUid;
        psDesc.fprogUid = fProgUid;
        psDesc.vertexLayout = vSource->ShaderVertexLayout();
        psDesc.blending = fSource->Blending();
        piplineState = rhi::AcquireRenderPipelineState(psDesc);
    }

    ShaderDescriptor* res = new ShaderDescriptor(piplineState, vProgUid, fProgUid);
    res->sourceName = name;
    res->defines = defines;
    res->valid = piplineState.IsValid(); //later add another conditions
    if (res->valid)
    {
        res->UpdateConfigFromSource(const_cast<rhi::ShaderSource*>(vSource), const_cast<rhi::ShaderSource*>(fSource));
        res->requiredVertexFormat = GetVertexLayoutRequiredFormat(psDesc.vertexLayout);
    }
    else
    {
        DAVA::Logger::Error("failed to get pipeline-state");
        DAVA::Logger::Info("  vprog-uid = %s", vProgUid.c_str());
        DAVA::Logger::Info("  fprog-uid = %s", fProgUid.c_str());
    }

    shaderDescriptors[key] = res;
    return res;
}

void ReloadShaders()
{
    DVASSERT(initialized);

    LockGuard<Mutex> guard(shaderCacheMutex);
    shaderSourceCodes.clear();
    rhi::ShaderSource::PurgeIncludesCache();

    //reload shaders
    for (auto& shaderDescr : shaderDescriptors)
    {
        ShaderDescriptor* shader = shaderDescr.second;

        /*Sources*/
        ShaderSourceCode sourceCode = GetSourceCode(shader->sourceName);
        rhi::ShaderSource vSource(sourceCode.vertexProgSourcePath.GetFrameworkPath().c_str());
        rhi::ShaderSource fSource(sourceCode.fragmentProgSourcePath.GetFrameworkPath().c_str());
        Vector<String> progDefines;
        progDefines.reserve(shader->defines.size() * 2);
        for (auto& it : shader->defines)
        {
            progDefines.push_back(String(it.first.c_str()));
            progDefines.push_back(DAVA::Format("%d", it.second));
        }
        vSource.Construct(rhi::PROG_VERTEX, sourceCode.vertexProgText.data(), progDefines);
        fSource.Construct(rhi::PROG_FRAGMENT, sourceCode.fragmentProgText.data(), progDefines);

        const std::string& vpBin = vSource.GetSourceCode(rhi::HostApi());
        rhi::ShaderCache::UpdateProgBinary(rhi::HostApi(), rhi::PROG_VERTEX, shader->vProgUid, vpBin.c_str(), unsigned(vpBin.length()));
        const std::string& fpBin = fSource.GetSourceCode(rhi::HostApi());
        rhi::ShaderCache::UpdateProgBinary(rhi::HostApi(), rhi::PROG_FRAGMENT, shader->fProgUid, fpBin.c_str(), unsigned(fpBin.length()));

        //ShaderDescr
        rhi::PipelineState::Descriptor psDesc;
        psDesc.vprogUid = shader->vProgUid;
        psDesc.fprogUid = shader->fProgUid;
        psDesc.vertexLayout = vSource.ShaderVertexLayout();
        psDesc.blending = fSource.Blending();
        rhi::ReleaseRenderPipelineState(shader->piplineState);
        shader->piplineState = rhi::AcquireRenderPipelineState(psDesc);
        shader->valid = shader->piplineState.IsValid(); //later add another conditions
        if (shader->valid)
        {
            shader->UpdateConfigFromSource(&vSource, &fSource);
            shader->requiredVertexFormat = GetVertexLayoutRequiredFormat(psDesc.vertexLayout);
        }
        else
        {
            shader->requiredVertexFormat = 0;
        }
    }
}
}
};
