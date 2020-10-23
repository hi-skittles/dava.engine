
#include "Tests/TexturesLoadingTest.h"

#include <Time/SystemTimer.h>
#include <Debug/ProfilerCPU.h>
#include <UI/Text/UITextComponent.h>
#include <UI/Events/UIEventBindingComponent.h>
#include <Render/Texture.h>
#include <thread>
#include <mutex>

#if defined(__DAVAENGINE_WINDOWS__)
#include <io.h>
#elif defined(__DAVAENGINE_POSIX__)
#include <unistd.h>
#endif
#include <sys/stat.h>

using namespace DAVA;

// Atlas images parameters
static const int32 LARGE_FILE_WIDTH = 2048;
static const int32 LARGE_FILE_HEIGHT = 2048;
static const int32 LARGE_FILES_COUNT = 1;

// Small file images parameters
static const int32 SMALL_FILE_WIDTH = 128;
static const int32 SMALL_FILE_HEIGHT = 128;
static const int32 SMALL_FILES_COUNT = 256;

TexturesLoadingTest::TexturesLoadingTest(TestBed& app)
    : BaseScreen(app, "TexturesLoadingTest")
{
}

void TexturesLoadingTest::LoadResources()
{
    using namespace DAVA;

    BaseScreen::LoadResources();

    GenerateTestFilePaths();

    DAVA::DefaultUIPackageBuilder pkgBuilder;
    DAVA::UIPackageLoader().LoadPackage("~res:/TestBed/UI/TexturesLoadingTest.yaml", &pkgBuilder);
    UIControl* dialog = pkgBuilder.GetPackage()->GetControl("MainFrame");
    AddControl(dialog);

    statusText = dialog->FindByName("StatusText")->GetOrCreateComponent<UITextComponent>();

    auto actions = dialog->GetOrCreateComponent<UIEventBindingComponent>();
    actions->BindAction(FastName("PREPARE"), [&](const DAVA::Any&) {
        if (!GetEngineContext()->fileSystem->Exists(workingDir))
        {
            int64 time = CreateFiles(false);
            statusText->SetText(Format("Files created! Time: %d ms", static_cast<int32>(time)));
        }
        else
        {
            statusText->SetText(Format("Files exist! Clean first."));
        }
    });
    actions->BindAction(FastName("PREPARE_NOISE"), [&](const DAVA::Any&) {
        if (!GetEngineContext()->fileSystem->Exists(workingDir))
        {
            int64 time = CreateFiles(true);
            statusText->SetText(Format("Files created! Time: %d ms", static_cast<int32>(time)));
        }
        else
        {
            statusText->SetText(Format("Files exist! Clean first."));
        }
    });
    actions->BindAction(FastName("CLEAN"), [&](const DAVA::Any&) {
        if (IsFilesPrepared())
        {
            int64 time = RemoveFiles();
            statusText->SetText(Format("Files removed! Time: %d ms", static_cast<int32>(time)));
        }
    });
    actions->BindAction(FastName("LOAD_ATLAS"), [&](const DAVA::Any&) {
        if (IsFilesPrepared())
        {
            DVASSERT(GetEngineContext()->fileSystem->Exists(workingDir));
            int64 time = LoadAtlas();
            statusText->SetText(Format("Atlas loaded! Time: %d(ms)", static_cast<int32>(time)));
        }
    });
    actions->BindAction(FastName("LOAD_FILES"), [&](const DAVA::Any&) {
        if (IsFilesPrepared())
        {
            int64 time = LoadSmallFiles();
            statusText->SetText(Format("Files loaded! Time: %d(ms)", static_cast<int32>(time)));
        }
    });
    actions->BindAction(FastName("LOAD_FILES_MT"), [&](const DAVA::Any&) {
        if (IsFilesPrepared())
        {
            DVASSERT(GetEngineContext()->fileSystem->Exists(workingDir));
            int64 time = LoadSmallFilesTwoThread();
            statusText->SetText(Format("Files loaded! Time: %d(ms)", static_cast<int32>(time)));
        }
    });
    actions->BindAction(FastName("LOAD_FILES_DATA"), [&](const DAVA::Any&) {
        if (IsFilesPrepared())
        {
            DVASSERT(GetEngineContext()->fileSystem->Exists(workingDir));
            int64 time = LoadSmallFilesBytes();
            statusText->SetText(Format("Bytes loaded! Time: %d(ms)", static_cast<int32>(time)));
        }
    });
}

void TexturesLoadingTest::UnloadResources()
{
    statusText = nullptr;
    BaseScreen::UnloadResources();
}

int64 TexturesLoadingTest::LoadAtlas()
{
    ProfilerCPU::globalProfiler->Start();
    int64 startTime = SystemTimer::GetMs();
    Vector<TextureHolder> holders;
    holders.reserve(largeFiles.size());

    {
        DAVA_PROFILER_CPU_SCOPE("TexturesLoadingTest_LoadAtlas");
        for (const TexturePaths& path : largeFiles)
        {
            TextureHolder holder(LoadTexture(path.descriptorPath));
            holders.emplace_back(holder);
        }
    }
    int64 time = (SystemTimer::GetMs() - startTime);

    ProfilerCPU::globalProfiler->Stop();
    Vector<TraceEvent> trace = ProfilerCPU::globalProfiler->GetTrace();
    TraceEvent::DumpJSON(trace, "~doc:/trace_LoadAtlas.json");
    Logger::Debug("[Test] Atlas time: %d(ms)", static_cast<int32>(time));
    return time;
}

int64 TexturesLoadingTest::LoadSmallFiles()
{
    ProfilerCPU::globalProfiler->Start();

    int64 startTime = SystemTimer::GetMs();
    Vector<TextureHolder> holders;
    holders.reserve(smallFiles.size());

    Vector<TexturePaths> smallFilesCopy = smallFiles;
    std::random_shuffle(smallFilesCopy.begin(), smallFilesCopy.end());
    {
        DAVA_PROFILER_CPU_SCOPE("TexturesLoadingTest_LoadSmallFiles");

        for (const TexturePaths& path : smallFilesCopy)
        {
            TextureHolder holder(LoadTexture(path.descriptorPath));
            holders.emplace_back(holder);
        }
    }
    int64 time = (SystemTimer::GetMs() - startTime);

    ProfilerCPU::globalProfiler->Stop();
    Vector<TraceEvent> trace = ProfilerCPU::globalProfiler->GetTrace();
    TraceEvent::DumpJSON(trace, "~doc:/trace_LoadSmallFiles.json");

    Logger::Debug("[Test] Small files time: %d(ms)", static_cast<int32>(time));
    return time;
}

int64 TexturesLoadingTest::LoadSmallFilesTwoThread()
{
    int64 startTime = SystemTimer::GetMs();
    Vector<TextureHolder> holders;
    holders.resize(smallFiles.size());

    std::mutex mtx;
    auto f = [&](int32 start, int32 stop)
    {
        for (int32 i = start; i < stop; i++)
        {
            TexturePaths& path = smallFiles[i];
            TextureHolder holder(LoadTexture(path.descriptorPath));
            mtx.lock();
            holders[i] = (holder);
            mtx.unlock();
        }
    };
    int32 midPoint = static_cast<int32>(smallFiles.size() / 2);
    std::thread t1(f, 0, midPoint);
    std::thread t2(f, midPoint, static_cast<int32>(smallFiles.size()));
    t1.join();
    t2.join();

    int64 time = (SystemTimer::GetMs() - startTime);
    Logger::Debug("[Test] Atlas time: %d(ms)", static_cast<int32>(time));
    return time;
}

int64 TexturesLoadingTest::LoadSmallFilesBytes()
{
    tmpReadBuffer.reserve(20 * 1024 * 1024);
    ProfilerCPU::globalProfiler->Start();

    int64 startTime = SystemTimer::GetMs();

    Vector<TexturePaths> smallFilesCopy = smallFiles;
    std::random_shuffle(smallFilesCopy.begin(), smallFilesCopy.end());
    {
        DAVA_PROFILER_CPU_SCOPE("TexturesLoadingTest_LoadFileBytes");

        for (const TexturePaths& path : smallFilesCopy)
        {
            DAVA_PROFILER_CPU_SCOPE("ReadDescriptorAndData");
            ProfileFileIO(path.descriptorPath);
            ProfileFileIO(path.texturePath);
        }
    }
    int64 time = (SystemTimer::GetMs() - startTime);

    ProfilerCPU::globalProfiler->Stop();
    Vector<TraceEvent> trace = ProfilerCPU::globalProfiler->GetTrace();
    TraceEvent::DumpJSON(trace, "~doc:/trace_PureFileIO.json");

    Logger::Debug("[Test] PureFileIO time: %d(ms)", static_cast<int32>(time));
    tmpReadBuffer.clear();
    tmpReadBuffer.shrink_to_fit();
    return time;
}

void TexturesLoadingTest::ProfileFileIO(const FilePath& path)
{
    DAVA_PROFILER_CPU_SCOPE("ProfileFileIO");
    FILE* pFile;
    {
        DAVA_PROFILER_CPU_SCOPE("fopen");
        pFile = fopen(path.GetAbsolutePathname().c_str(), "rb");
    }
    DVASSERT(pFile != NULL);

    int64 size = 0;
    const bool use_fstat = false;

#if defined(__DAVAENGINE_WINDOWS__)
    struct _stat st;
    if (use_fstat)
    {
        DAVA_PROFILER_CPU_SCOPE("fstat");
        _fstat(_fileno(pFile), &st);
        size = st.st_size;
    }
    else
    {
        {
            DAVA_PROFILER_CPU_SCOPE("fseek_end");
            _fseeki64(pFile, 0, SEEK_END);
        }
        {
            DAVA_PROFILER_CPU_SCOPE("ftell");
            size = _ftelli64(pFile);
        }
        {
            DAVA_PROFILER_CPU_SCOPE("fseek_begin");
            _fseeki64(pFile, 0, SEEK_SET);
        }
    }
#else
    struct stat st;
    if (use_fstat)
    {
        DAVA_PROFILER_CPU_SCOPE("fstat");
        fstat(fileno(pFile), &st);
        size = st.st_size;
    }
    else
    {
        {
            DAVA_PROFILER_CPU_SCOPE("fseek_end");
            fseeko(pFile, 0, SEEK_END);
        }
        {
            DAVA_PROFILER_CPU_SCOPE("ftell");
            size = ftello(pFile);
        }
        {
            DAVA_PROFILER_CPU_SCOPE("fseek_begin");
            fseeko(pFile, 0, SEEK_SET);
        }
    }
#endif

    tmpReadBuffer.reserve(static_cast<size_t>(size));

    int64 rs = 0;
    {
        DAVA_PROFILER_CPU_SCOPE("fread");
        rs = fread(tmpReadBuffer.data(), 1, static_cast<size_t>(size), pFile);
    }
    DVASSERT(rs == size);

    {
        DAVA_PROFILER_CPU_SCOPE("fclose");
        fclose(pFile);
    }
}

int64 TexturesLoadingTest::RemoveFiles()
{
    int64 startTime = SystemTimer::GetMs();
    GetEngineContext()->fileSystem->DeleteDirectory(workingDir, true);
    int64 time = (SystemTimer::GetMs() - startTime);
    Logger::Debug("[Test] Files removed time: %d ms", static_cast<int32>(time));
    return time;
}

void TexturesLoadingTest::GenerateTestFilePaths()
{
#ifdef __DAVAENGINE_ANDROID__
    List<DeviceInfo::StorageInfo> storageList = DeviceInfo::GetStoragesList();
    DeviceInfo::StorageInfo external;
    bool externalStorage(false);
    for (const auto& it : storageList)
    {
        if (DeviceInfo::STORAGE_TYPE_PRIMARY_EXTERNAL == it.type)
        {
            external = it;
            DVASSERT(false == externalStorage);
            externalStorage = true;
        }
    }
    workingDir = external.path + "TEXTURES/";
#else
    workingDir = "~doc:/TEXTURES/";
#endif

    TextureDescriptor descriptor;
    descriptor.imageFormat = ImageFormat::IMAGE_FORMAT_WEBP;
    descriptor.format = PixelFormat::FORMAT_RGBA8888;
    descriptor.dataSettings.sourceFileFormat = ImageFormat::IMAGE_FORMAT_WEBP;
    descriptor.dataSettings.sourceFileExtension = ".webp";

    largeFiles.resize(LARGE_FILES_COUNT);
    for (int32 idx = 0; idx < LARGE_FILES_COUNT; idx++)
    {
        descriptor.pathname = workingDir + Format("tmp_atlas_%03d.tex", idx);

        largeFiles[idx].descriptorPath = descriptor.pathname;
        largeFiles[idx].texturePath = descriptor.CreateMultiMipPathnameForGPU(eGPUFamily::GPU_ORIGIN);
    }

    smallFiles.resize(SMALL_FILES_COUNT);
    for (int32 idx = 0; idx < SMALL_FILES_COUNT; idx++)
    {
        descriptor.pathname = workingDir + Format("tmp_tex_%03d.tex", idx);

        smallFiles[idx].descriptorPath = descriptor.pathname;
        smallFiles[idx].texturePath = descriptor.CreateMultiMipPathnameForGPU(eGPUFamily::GPU_ORIGIN);
    }
}

int64 TexturesLoadingTest::CreateFiles(bool generateNoiseImage)
{
    int64 startTime = SystemTimer::GetMs();

    GetEngineContext()->fileSystem->CreateDirectory(workingDir);
    FilePath tempPath = workingDir + "Temp/";
    GetEngineContext()->fileSystem->CreateDirectory(tempPath);

    FilePath atlasFile;
    FilePath smallFile;
    if (generateNoiseImage)
    {
        atlasFile = tempPath + "atlas.webp";
        smallFile = tempPath + "small.webp";
        CreateWebPFile(atlasFile, LARGE_FILE_WIDTH, LARGE_FILE_HEIGHT);
        CreateWebPFile(smallFile, SMALL_FILE_WIDTH, SMALL_FILE_HEIGHT);
    }
    else
    {
        atlasFile = "~res:/TestBed/TestData/TextureLoadingTest/2048x2048.webp";
        smallFile = "~res:/TestBed/TestData/TextureLoadingTest/128x128.webp";
    }

    for (const TexturePaths& path : largeFiles)
    {
        FilePath baseDir = path.descriptorPath.GetDirectory();
        GetEngineContext()->fileSystem->CreateDirectory(baseDir);
        bool res = GetEngineContext()->fileSystem->CopyFile(atlasFile, path.texturePath);
        DVASSERT(res);
        CreateTextureDescriptorFile(path.descriptorPath, LARGE_FILE_WIDTH, LARGE_FILE_HEIGHT);
    }

    for (const TexturePaths& path : smallFiles)
    {
        FilePath baseDir = path.descriptorPath.GetDirectory();
        GetEngineContext()->fileSystem->CreateDirectory(baseDir);
        bool res = GetEngineContext()->fileSystem->CopyFile(smallFile, path.texturePath);
        DVASSERT(res);
        CreateTextureDescriptorFile(path.descriptorPath, SMALL_FILE_WIDTH, SMALL_FILE_HEIGHT);
    }
    int64 time = (SystemTimer::GetMs() - startTime);
    Logger::Debug("[Test] Generate images time: %d ms", static_cast<int32>(time));
    return time;
}

bool TexturesLoadingTest::IsFilesPrepared() const
{
    bool result = GetEngineContext()->fileSystem->Exists(workingDir);
    if (!result)
    {
        statusText->SetText(Format("Prepare files first!"));
    }
    return result;
};

void TexturesLoadingTest::CreateWebPFile(const FilePath& imagePath, int32 w, int32 h) const
{
    ScopedPtr<Image> image(Image::Create(w, h, PixelFormat::FORMAT_RGBA8888));
    for (uint32 offset = 0; offset < image->dataSize; offset++)
    {
        image->data[offset] = rand();
    }
    image->Save(imagePath);
}

void TexturesLoadingTest::CreateTextureDescriptorFile(const FilePath& descriptorPath, int32 w, int32 h) const
{
    TextureDescriptor descriptor;
    descriptor.Initialize(rhi::TEXADDR_CLAMP, false);
    descriptor.dataSettings.sourceFileFormat = ImageFormat::IMAGE_FORMAT_WEBP;
    descriptor.dataSettings.sourceFileExtension = ".webp";

    for (uint8 i = 0; i < eGPUFamily::GPU_FAMILY_COUNT; ++i)
    {
        descriptor.compression[i].imageFormat = ImageFormat::IMAGE_FORMAT_WEBP;
        descriptor.compression[i].format = PixelFormat::FORMAT_RGBA8888;
        descriptor.compression[i].compressToWidth = w;
        descriptor.compression[i].compressToHeight = h;
    }

    descriptor.Export(descriptorPath, eGPUFamily::GPU_ORIGIN);
}

TexturesLoadingTest::TextureHolder TexturesLoadingTest::LoadTexture(const FilePath& path) const
{
    auto holder = RefPtr<Texture>(Texture::CreateFromFile(path));
    DVASSERT(holder->isPink == false);
    return holder;
}
