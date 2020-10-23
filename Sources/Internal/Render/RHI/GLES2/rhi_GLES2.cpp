#include "rhi_GLES2.h"

#include "../rhi_Public.h"

#include "../Common/rhi_Private.h"
#include "../Common/rhi_Pool.h"
#include "../Common/rhi_Utils.h"
#include "../Common/dbg_StatSet.h"
#include "../Common/rhi_CommonImpl.h"

#include "Debug/DVAssert.h"
#include "Logger/Logger.h"
using DAVA::Logger;

#include <cstring>

#include "_gl.h"

GLuint _GLES2_Bound_FrameBuffer = 0;
GLuint _GLES2_Default_FrameBuffer = 0;
void* _GLES2_Native_Window = nullptr;
void* _GLES2_Context = nullptr;
void (*_GLES2_AcquireContext)() = nullptr;
void (*_GLES2_ReleaseContext)() = nullptr;
int _GLES2_DefaultFrameBuffer_Width = 0;
int _GLES2_DefaultFrameBuffer_Height = 0;
GLuint _GLES2_LastSetIB = 0;
DAVA::uint8* _GLES2_LastSetIndices = nullptr;
GLuint _GLES2_LastSetVB = 0;
GLuint _GLES2_LastSetTex0 = 0;
GLenum _GLES2_LastSetTex0Target = GL_TEXTURE_2D;
int _GLES2_LastActiveTexture = -1;
bool _GLES2_IsDebugSupported = true;
bool _GLES2_IsGlDepth24Stencil8Supported = true;
bool _GLES2_IsGlDepthNvNonLinearSupported = false;
bool _GLES2_IsSeamlessCubmapSupported = false;
bool _GLES2_UseUserProvidedIndices = false;
bool _GLES2_TimeStampQuerySupported = false;
volatile bool _GLES2_ValidateNeonCalleeSavedRegisters = false;

#if defined(__DAVAENGINE_ANDROID__) && defined(__DAVAENGINE_ARM_7__)
volatile GLCallRegisters gl_call_registers;
#endif


#define ENABLE_DEBUG_OUTPUT 0

namespace rhi
{
//==============================================================================

Dispatch DispatchGLES2 = {};

static bool ATC_Supported = false;
static bool PVRTC_Supported = false;
static bool PVRTC2_Supported = false;
static bool ETC1_Supported = false;
static bool ETC2_Supported = false;
static bool EAC_Supported = false;
static bool DXT_Supported = false;
static bool Float_Supported = false;
static bool Half_Supported = false;
static bool RG_Supported = false;
static bool Short_Int_Supported = false;

//------------------------------------------------------------------------------

static Api gles2_HostApi()
{
    return RHI_GLES2;
}

//------------------------------------------------------------------------------

static bool gles2_TextureFormatSupported(TextureFormat format, ProgType)
{
    bool supported = false;

    switch (format)
    {
    case TEXTURE_FORMAT_R8G8B8A8:
    case TEXTURE_FORMAT_R8G8B8:
    case TEXTURE_FORMAT_R5G5B5A1:
    case TEXTURE_FORMAT_R5G6B5:
    case TEXTURE_FORMAT_R4G4B4A4:
    case TEXTURE_FORMAT_R8:
    case TEXTURE_FORMAT_R16:
        supported = true;
        break;

    case TEXTURE_FORMAT_DXT1:
    case TEXTURE_FORMAT_DXT3:
    case TEXTURE_FORMAT_DXT5:
        supported = DXT_Supported;
        break;

    case TEXTURE_FORMAT_A16R16G16B16:
    case TEXTURE_FORMAT_A32R32G32B32:
        supported = Short_Int_Supported;
        break;

    case TEXTURE_FORMAT_RGBA16F:
        supported = Half_Supported;
        break;

    case TEXTURE_FORMAT_RGBA32F:
        supported = Float_Supported;
        break;

    case TEXTURE_FORMAT_R16F:
    case TEXTURE_FORMAT_RG16F:
        supported = Half_Supported && RG_Supported;
        break;

    case TEXTURE_FORMAT_R32F:
    case TEXTURE_FORMAT_RG32F:
        supported = Float_Supported && RG_Supported;
        break;

    case TEXTURE_FORMAT_PVRTC_4BPP_RGBA:
    case TEXTURE_FORMAT_PVRTC_2BPP_RGBA:
        supported = PVRTC_Supported;
        break;

    case TEXTURE_FORMAT_PVRTC2_4BPP_RGB:
    case TEXTURE_FORMAT_PVRTC2_4BPP_RGBA:
    case TEXTURE_FORMAT_PVRTC2_2BPP_RGB:
    case TEXTURE_FORMAT_PVRTC2_2BPP_RGBA:
        supported = PVRTC2_Supported;
        break;

    case TEXTURE_FORMAT_ATC_RGB:
    case TEXTURE_FORMAT_ATC_RGBA_EXPLICIT:
    case TEXTURE_FORMAT_ATC_RGBA_INTERPOLATED:
        supported = ATC_Supported;
        break;

    case TEXTURE_FORMAT_ETC1:
        supported = ETC1_Supported;
        break;

    case TEXTURE_FORMAT_ETC2_R8G8B8:
    case TEXTURE_FORMAT_ETC2_R8G8B8A8:
    case TEXTURE_FORMAT_ETC2_R8G8B8A1:
        supported = ETC2_Supported;
        break;

    case TEXTURE_FORMAT_EAC_R11_UNSIGNED:
    case TEXTURE_FORMAT_EAC_R11_SIGNED:
    case TEXTURE_FORMAT_EAC_R11G11_UNSIGNED:
    case TEXTURE_FORMAT_EAC_R11G11_SIGNED:
        supported = EAC_Supported;
        break;

    default:
        supported = false;
    }

    return supported;
}

static void gles_check_GL_extensions()
{
    const char* ext = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));

    if (!IsEmptyString(ext))
    {
        ATC_Supported = strstr(ext, "GL_AMD_compressed_ATC_texture") != nullptr;
        PVRTC_Supported = strstr(ext, "GL_IMG_texture_compression_pvrtc") != nullptr;
        PVRTC2_Supported = strstr(ext, "GL_IMG_texture_compression_pvrtc2") != nullptr;
        ETC1_Supported = strstr(ext, "GL_OES_compressed_ETC1_RGB8_texture") != nullptr;
        ETC2_Supported = strstr(ext, "GL_OES_compressed_ETC2_RGB8_texture") != nullptr;
        EAC_Supported = ETC2_Supported;
        DXT_Supported = (strstr(ext, "GL_EXT_texture_compression_s3tc") != nullptr) || (strstr(ext, "GL_NV_texture_compression_s3tc") != nullptr);

        Float_Supported = strstr(ext, "GL_OES_texture_float") != nullptr || strstr(ext, "ARB_texture_float") != nullptr;
        Half_Supported = strstr(ext, "GL_OES_texture_half_float") != nullptr || strstr(ext, "ARB_texture_float") != nullptr;
        RG_Supported = strstr(ext, "EXT_texture_rg") != nullptr || strstr(ext, "ARB_texture_rg") != nullptr;

        MutableDeviceCaps::Get().is32BitIndicesSupported = strstr(ext, "GL_OES_element_index_uint") != nullptr;
        MutableDeviceCaps::Get().isVertexTextureUnitsSupported = strstr(ext, "GL_EXT_shader_texture_lod") != nullptr;
        MutableDeviceCaps::Get().isFramebufferFetchSupported = strstr(ext, "GL_EXT_shader_framebuffer_fetch") != nullptr;
        MutableDeviceCaps::Get().isInstancingSupported =
        (strstr(ext, "GL_EXT_draw_instanced") || strstr(ext, "GL_ARB_draw_instanced") || strstr(ext, "GL_ARB_draw_elements_base_vertex")) &&
        (strstr(ext, "GL_EXT_instanced_arrays") || strstr(ext, "GL_ARB_instanced_arrays"));

#if defined(__DAVAENGINE_ANDROID__)
        _GLES2_IsGlDepth24Stencil8Supported = (strstr(ext, "GL_DEPTH24_STENCIL8") != nullptr) || (strstr(ext, "GL_OES_packed_depth_stencil") != nullptr) || (strstr(ext, "GL_EXT_packed_depth_stencil") != nullptr);
#else
        _GLES2_IsGlDepth24Stencil8Supported = true;
#endif

        _GLES2_IsDebugSupported = strstr(ext, "GL_KHR_debug") != nullptr;
        _GLES2_IsGlDepthNvNonLinearSupported = strstr(ext, "GL_DEPTH_COMPONENT16_NONLINEAR_NV") != nullptr;
        _GLES2_IsSeamlessCubmapSupported = strstr(ext, "GL_ARB_seamless_cube_map") != nullptr;

#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_MACOS__)
        if (_GLES2_IsSeamlessCubmapSupported)
            glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
#endif
        if (strstr(ext, "EXT_texture_filter_anisotropic") != nullptr)
        {
            float32 value = 0.0f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &value);
            MutableDeviceCaps::Get().maxAnisotropy = static_cast<DAVA::uint32>(value);
        }

        _GLES2_TimeStampQuerySupported = strstr(ext, "ARB_timer_query") != nullptr || strstr(ext, "EXT_disjoint_timer_query") != nullptr;
        MutableDeviceCaps::Get().isPerfQuerySupported = strstr(ext, "EXT_timer_query") != nullptr || _GLES2_TimeStampQuerySupported;
    }

    int majorVersion = 2;
    int minorVersion = 0;
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (!IsEmptyString(version))
    {
        DAVA::Logger::Info("OpenGL version: %s", version);

        const char* dotChar = strchr(version, '.');
        if (dotChar && dotChar != version && *(dotChar + 1))
        {
            majorVersion = atoi(dotChar - 1);
            minorVersion = atoi(dotChar + 1);
        }

        if (strstr(version, "OpenGL ES"))
        {
            if (majorVersion >= 3)
            {
                MutableDeviceCaps::Get().is32BitIndicesSupported = true;
                MutableDeviceCaps::Get().isVertexTextureUnitsSupported = true;
                MutableDeviceCaps::Get().isInstancingSupported = true;
                Short_Int_Supported = true;
            }

#ifdef __DAVAENGINE_ANDROID__
            GET_GL_FUNC(glDrawElementsInstanced, "EXT");
            GET_GL_FUNC(glDrawArraysInstanced, "EXT");
            GET_GL_FUNC(glVertexAttribDivisor, "EXT");

            GET_GL_FUNC(glRenderbufferStorageMultisample, "EXT");
            GET_GL_FUNC(glBlitFramebuffer, "EXT");

            GET_GL_FUNC(glDebugMessageControl, "KHR");
            GET_GL_FUNC(glDebugMessageCallback, "KHR");

            GET_GL_FUNC(glGenQueries, "EXT");
            GET_GL_FUNC(glDeleteQueries, "EXT");
            GET_GL_FUNC(glBeginQuery, "EXT");
            GET_GL_FUNC(glEndQuery, "EXT");
            GET_GL_FUNC(glGetQueryObjectuiv, "EXT");
            GET_GL_FUNC(glQueryCounter, "EXT");
            GET_GL_FUNC(glGetQueryObjectui64v, "EXT");
#endif
        }
        else
        {
            MutableDeviceCaps::Get().is32BitIndicesSupported = true;
            MutableDeviceCaps::Get().isVertexTextureUnitsSupported = true;
            MutableDeviceCaps::Get().isFramebufferFetchSupported = false;
            MutableDeviceCaps::Get().isInstancingSupported |= (majorVersion > 3) && (minorVersion > 3);

            if (majorVersion >= 3)
            {
                if ((majorVersion > 3) || (minorVersion >= 2))
                    _GLES2_IsSeamlessCubmapSupported = true;

                if ((majorVersion > 3) || (minorVersion >= 3))
                {
                    _GLES2_TimeStampQuerySupported = true;
                    MutableDeviceCaps::Get().isPerfQuerySupported = true;
                }
            }

            Float_Supported |= majorVersion >= 3;
            Half_Supported |= majorVersion >= 3;
            RG_Supported |= majorVersion >= 3;
            Short_Int_Supported = true;
        }
    }

#ifdef __DAVAENGINE_WIN32__
    MutableDeviceCaps::Get().isInstancingSupported = (glVertexAttribDivisor != nullptr);
#endif
    
#ifdef __DAVAENGINE_ANDROID__
    if (_GLES2_TimeStampQuerySupported)
    {
        //some driver returns available timestamp-query extension but not implement it

        GLuint query = 0;
        if (glGenQueries)
            GL_CALL(glGenQueries(1, &query));

        if (query)
        {
            GLuint64 ts = 0;

            if (glQueryCounter)
                GL_CALL(glQueryCounter(query, GL_TIMESTAMP));

            GL_CALL(glFinish());

            if (glGetQueryObjectui64v)
                GL_CALL(glGetQueryObjectui64v(query, GL_QUERY_RESULT, &ts));

            if (glDeleteQueries)
                GL_CALL(glDeleteQueries(1, &query));

            MutableDeviceCaps::Get().isPerfQuerySupported = _GLES2_TimeStampQuerySupported = (ts != 0);
        }
    }
#endif

    bool runningOnTegra = false;
    bool runningOnMali = false;
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    if (!IsEmptyString(renderer))
    {
        std::strncpy(MutableDeviceCaps::Get().deviceDescription, renderer, 127);
        runningOnMali = strstr(renderer, "Mali") != nullptr;
        runningOnTegra = strcmp(renderer, "NVIDIA Tegra") == 0;
    }

    GLint maxSamples = 1;
    
#ifdef __DAVAENGINE_ANDROID__ // hacks and workarounds for beautiful Android

    // drawing from memory is worst case scenario,
    // unless running on some buggy piece of shit
    _GLES2_UseUserProvidedIndices = runningOnMali;

    // Without offensive language:
    // it seems like some GL-functions in SHIELD driver implementation
    // corrupt 'callee-saved' Neon registers (q4-q7).
    // So, we just restore it after any GL-call.
    _GLES2_ValidateNeonCalleeSavedRegisters = runningOnTegra;

    // allow multisampling only on NVIDIA Tegra GPU
    // and if functions were loaded
    if (runningOnTegra && (majorVersion >= 3) && (glRenderbufferStorageMultisample != nullptr) && (glBlitFramebuffer != nullptr))
#endif
    {
        GL_CALL(glGetIntegerv(GL_MAX_SAMPLES, &maxSamples));
        DAVA::Logger::Info("GL_MAX_SAMPLES -> %d", maxSamples);
    }

#ifdef __DAVAENGINE_MACOS__
    if (strstr(renderer, "Radeon HD 2400") != nullptr ||
        strstr(renderer, "Radeon HD 2600") != nullptr)
    {
        maxSamples = 1;
    }
#endif

    MutableDeviceCaps::Get().maxSamples = static_cast<uint32>(maxSamples);

    GLint maxTextureSize = 1024;
    GL_CALL(glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize));
    MutableDeviceCaps::Get().maxTextureSize = maxTextureSize;
}

//------------------------------------------------------------------------------

void gles2_Uninitialize()
{
    //TODO: release GL resources
    //now it's crash cause Qt context deleted before uninit renderer
    //QueryBufferGLES2::ReleaseQueryObjectsPool();
    //PerfQueryGLES2::ReleaseQueryObjectsPool();
}

//------------------------------------------------------------------------------

static void gles2_Reset(const ResetParam& param)
{
#if defined(__DAVAENGINE_ANDROID__)
    android_gl_reset(param.window, GLint(param.width), GLint(param.height));
#elif defined(__DAVAENGINE_IPHONE__)
    ios_gl_reset(param.window, GLint(param.width), GLint(param.height));
#elif defined(__DAVAENGINE_MACOS__)
    macos_gl_reset(param);
#elif defined(__DAVAENGINE_WIN32__)
    win_gl_reset(param);
#else
    _GLES2_DefaultFrameBuffer_Width = param.width;
    _GLES2_DefaultFrameBuffer_Height = param.height;
#endif
}

//------------------------------------------------------------------------------

static void gles2_InvalidateCache()
{
    PipelineStateGLES2::InvalidateCache();
    DepthStencilStateGLES2::InvalidateCache();
    TextureGLES2::InvalidateCache();
}

static void gles2_SynchronizeCPUGPU(uint64* cpuTimestamp, uint64* gpuTimestamp)
{
    GLCommand cmd = { GLCommand::SYNC_CPU_GPU, { uint64(cpuTimestamp), uint64(gpuTimestamp) } };
    ExecGL(&cmd, 1);
}

//------------------------------------------------------------------------------

static bool
gles2_NeedRestoreResources()
{
    bool needRestore = TextureGLES2::NeedRestoreCount() || VertexBufferGLES2::NeedRestoreCount() || IndexBufferGLES2::NeedRestoreCount();

    return needRestore;
}

static bool gles2_CheckSurface()
{    
#if defined __DAVAENGINE_ANDROID__
    return android_gl_checkSurface();
#elif defined __DAVAENGINE_IPHONE__
    return ios_gl_check_layer();
#else
    return true;
#endif
}

static void gles2_Suspend()
{
    GL_CALL(glFinish());
}


#if defined(__DAVAENGINE_WIN32__)
void GLAPIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userdata)
{
#if 0    
    const char* ssource     = "unknown";
    const char* stype       = "unknown";
    const char* sseverity   = "unknown";

    switch( source )
    {
    case GL_DEBUG_SOURCE_API                : ssource = "API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM      : ssource = "window system"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER    : ssource = "shader compiler"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY        : ssource = "third party"; break;
    case GL_DEBUG_SOURCE_APPLICATION        : ssource = "application"; break;
    case GL_DEBUG_SOURCE_OTHER              : ssource = "other"; break;
    default                                 : ssource= "unknown"; break;
    }

    switch( type )
    {
    case GL_DEBUG_TYPE_ERROR                : stype = "error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR  : stype = "deprecated behavior"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR   : stype = "undefined behavior"; break;
    case GL_DEBUG_TYPE_PORTABILITY          : stype = "portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE          : stype = "performance"; break;
    case GL_DEBUG_TYPE_OTHER                : stype = "other"; break;
    default                                 : stype = "unknown"; break;
    }

    switch( severity )
    {
    case GL_DEBUG_SEVERITY_HIGH             : sseverity = "high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM           : sseverity = "medium"; break;
    case GL_DEBUG_SEVERITY_LOW              : sseverity = "low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION     : sseverity = "notification"; break;
    default                                 : sseverity = "unknown"; break;
    }
#endif
    if (type == GL_DEBUG_TYPE_PERFORMANCE)
        Trace("[gl.warning] %s\n", message);
    else if (type == GL_DEBUG_TYPE_ERROR)
        Trace("[gl.error] %s\n", message);
    //    else
    //        Logger::Info( "[gl] %s\n", message );
}
#elif defined(__DAVAENGINE_ANDROID__)
void GL_APIENTRY android_gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userdata)
{
    if ((message != nullptr) && (length > 1))
    {
        DAVA::Logger::Info("OpenGL debug message (%d): %s", length, message);
    }
}
#endif

void gles2_enable_debug_output()
{
#if ENABLE_DEBUG_OUTPUT
    DVASSERT(_GLES2_IsDebugSupported);

    DAVA::Logger::Error("Enabling OpenGL debug...");

#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_ANDROID__)
    GL_CALL(glEnable(GL_DEBUG_OUTPUT));
    GL_CALL(glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE));
    GL_CALL(glDebugMessageCallback(&gl_debug_callback, nullptr));    
#else
#error DEBUG_OUTPUT Not implemented for platform
#endif

#endif
}
//------------------------------------------------------------------------------

void gles2_Initialize(const InitParam& param)
{
    _GLES2_DefaultFrameBuffer_Width = param.width;
    _GLES2_DefaultFrameBuffer_Height = param.height;
    _GLES2_Default_FrameBuffer = param.defaultFrameBuffer ? static_cast<GLuint>(reinterpret_cast<intptr_t>(param.defaultFrameBuffer)) : 0;

#if defined(__DAVAENGINE_WIN32__)
    win32_gl_init(param);
    _GLES2_AcquireContext = (param.acquireContextFunc) ? param.acquireContextFunc : &win32_gl_acquire_context;
    _GLES2_ReleaseContext = (param.releaseContextFunc) ? param.releaseContextFunc : &win32_gl_release_context;    
#elif defined(__DAVAENGINE_MACOS__)
    macos_gl_init(param);
    _GLES2_AcquireContext = (param.acquireContextFunc) ? param.acquireContextFunc : &macos_gl_acquire_context;
    _GLES2_ReleaseContext = (param.releaseContextFunc) ? param.releaseContextFunc : &macos_gl_release_context;    
#elif defined(__DAVAENGINE_IPHONE__)
    ios_gl_init(param.window);
    _GLES2_AcquireContext = (param.acquireContextFunc) ? param.acquireContextFunc : &ios_gl_acquire_context;
    _GLES2_ReleaseContext = (param.releaseContextFunc) ? param.releaseContextFunc : &ios_gl_release_context;
#elif defined(__DAVAENGINE_ANDROID__)
    android_gl_init(param.window);
    _GLES2_AcquireContext = (param.acquireContextFunc) ? param.acquireContextFunc : &android_gl_acquire_context;
    _GLES2_ReleaseContext = (param.releaseContextFunc) ? param.releaseContextFunc : &android_gl_release_context;                
#endif

    if (param.maxVertexBufferCount)
        VertexBufferGLES2::Init(param.maxVertexBufferCount);
    if (param.maxIndexBufferCount)
        IndexBufferGLES2::Init(param.maxIndexBufferCount);
    if (param.maxConstBufferCount)
        ConstBufferGLES2::Init(param.maxConstBufferCount);
    if (param.maxTextureCount)
        TextureGLES2::Init(param.maxTextureCount);

    if (param.maxSamplerStateCount)
        SamplerStateGLES2::Init(param.maxSamplerStateCount);
    if (param.maxPipelineStateCount)
        PipelineStateGLES2::Init(param.maxPipelineStateCount);
    if (param.maxDepthStencilStateCount)
        DepthStencilStateGLES2::Init(param.maxDepthStencilStateCount);
    if (param.maxRenderPassCount)
        RenderPassGLES2::Init(param.maxRenderPassCount);
    if (param.maxCommandBuffer)
        CommandBufferGLES2::Init(param.maxCommandBuffer);

    uint32 ringBufferSize = 4 * 1024 * 1024;
    if (param.shaderConstRingBufferSize)
        ringBufferSize = param.shaderConstRingBufferSize;
    ConstBufferGLES2::InitializeRingBuffer(ringBufferSize);

    Logger::FrameworkDebug("GL initialized");
    Logger::FrameworkDebug("  GL version   : %s", glGetString(GL_VERSION));
    Logger::FrameworkDebug("  GPU vendor   : %s", glGetString(GL_VENDOR));
    Logger::FrameworkDebug("  GPU          : %s", glGetString(GL_RENDERER));
    Logger::FrameworkDebug("  GLSL version : %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    VertexBufferGLES2::SetupDispatch(&DispatchGLES2);
    IndexBufferGLES2::SetupDispatch(&DispatchGLES2);
    QueryBufferGLES2::SetupDispatch(&DispatchGLES2);
    PerfQueryGLES2::SetupDispatch(&DispatchGLES2);
    TextureGLES2::SetupDispatch(&DispatchGLES2);
    PipelineStateGLES2::SetupDispatch(&DispatchGLES2);
    ConstBufferGLES2::SetupDispatch(&DispatchGLES2);
    DepthStencilStateGLES2::SetupDispatch(&DispatchGLES2);
    SamplerStateGLES2::SetupDispatch(&DispatchGLES2);
    RenderPassGLES2::SetupDispatch(&DispatchGLES2);
    CommandBufferGLES2::SetupDispatch(&DispatchGLES2);

    DispatchGLES2.impl_Reset = &gles2_Reset;
    DispatchGLES2.impl_Uninitialize = &gles2_Uninitialize;
    DispatchGLES2.impl_HostApi = &gles2_HostApi;
    DispatchGLES2.impl_TextureFormatSupported = &gles2_TextureFormatSupported;
    DispatchGLES2.impl_NeedRestoreResources = &gles2_NeedRestoreResources;
    DispatchGLES2.impl_InvalidateCache = &gles2_InvalidateCache;
    DispatchGLES2.impl_SyncCPUGPU = &gles2_SynchronizeCPUGPU;

    DispatchGLES2.impl_InitContext = _GLES2_AcquireContext;
    DispatchGLES2.impl_ValidateSurface = &gles2_CheckSurface;
    DispatchGLES2.impl_FinishRendering = &gles2_Suspend;

    SetDispatchTable(DispatchGLES2);

    gles_check_GL_extensions();    
    

#if ENABLE_DEBUG_OUTPUT
    gles2_enable_debug_output();
#endif

    stat_DIP = StatSet::AddStat("rhi'dip", "dip");
    stat_DP = StatSet::AddStat("rhi'dp", "dp");
    stat_DTL = StatSet::AddStat("rhi'dtl", "dtl");
    stat_DTS = StatSet::AddStat("rhi'dts", "dts");
    stat_DLL = StatSet::AddStat("rhi'dll", "dll");
    stat_SET_PS = StatSet::AddStat("rhi'set-ps", "set-ps");
    stat_SET_SS = StatSet::AddStat("rhi'set-ss", "set-ss");
    stat_SET_TEX = StatSet::AddStat("rhi'set-tex", "set-tex");
    stat_SET_CB = StatSet::AddStat("rhi'set-cb", "set-cb");
    stat_SET_VB = StatSet::AddStat("rhi'set-vb", "set-vb");
    stat_SET_IB = StatSet::AddStat("rhi'set-ib", "set-ib");

    if (param.threadedRenderEnabled)
        _GLES2_ReleaseContext();
}

} // namespace rhi

#define CASE_TO_STRING(x) case x: return #x;
const char* glErrorToString(GLint error)
{
    switch (error)
    {
        CASE_TO_STRING(GL_NO_ERROR)
        CASE_TO_STRING(GL_INVALID_ENUM)
        CASE_TO_STRING(GL_INVALID_VALUE)
        CASE_TO_STRING(GL_INVALID_OPERATION)
        CASE_TO_STRING(GL_INVALID_FRAMEBUFFER_OPERATION)
        CASE_TO_STRING(GL_OUT_OF_MEMORY)

    default:
        return "Unknown OpenGL error";
    };
}
#undef CASE_TO_STRING

GLint GetGLRenderTargetFormat(rhi::TextureFormat rhiFormat)
{
    switch (rhiFormat)
    {
    case rhi::TEXTURE_FORMAT_R8G8B8A8:
        return GL_RGBA8;

    case rhi::TEXTURE_FORMAT_R5G6B5:
    {
    #if defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_WINDOWS__)
        return GL_RGB5;
    #else
        return GL_RGB565;
    #endif
    }

    default:
        DVASSERT(0, "Unsupported or unknown render target format specified");
        return 0;
    }
}

bool GetGLTextureFormat(rhi::TextureFormat rhiFormat, GLint* internalFormat, GLint* format, GLenum* type, bool* compressed)
{
    using namespace rhi;

    bool success = false;

    switch (rhiFormat)
    {
    case TEXTURE_FORMAT_R8G8B8A8:
        *internalFormat = GL_RGBA;
        *format = GL_RGBA;
        *type = GL_UNSIGNED_BYTE;
        *compressed = false;
        success = true;
        break;

    case TEXTURE_FORMAT_R8G8B8:
        *internalFormat = GL_RGB;
        *format = GL_RGB;
        *type = GL_UNSIGNED_BYTE;
        *compressed = false;
        success = true;
        break;

    case TEXTURE_FORMAT_R4G4B4A4:
        *internalFormat = GL_RGBA;
        *format = GL_RGBA;
        *type = GL_UNSIGNED_SHORT_4_4_4_4;
        *compressed = false;
        success = true;
        break;

    case TEXTURE_FORMAT_R8:
        *internalFormat = GL_ALPHA;
        *format = GL_ALPHA;
        *type = GL_UNSIGNED_BYTE;
        *compressed = false;
        success = true;
        break;

    case TEXTURE_FORMAT_R16:
        *internalFormat = GL_ALPHA;
        *format = GL_ALPHA;
        *type = GL_UNSIGNED_SHORT;
        *compressed = false;
        success = true;
        break;

    case TEXTURE_FORMAT_R5G5B5A1:
        *internalFormat = GL_RGBA;
        *format = GL_RGBA;
        *type = GL_UNSIGNED_SHORT_5_5_5_1;
        *compressed = false;
        success = true;
        break;

    case TEXTURE_FORMAT_R5G6B5:
        *internalFormat = GL_RGB;
        *format = GL_RGB;
        *type = GL_UNSIGNED_SHORT_5_6_5;
        *compressed = false;
        success = true;
        break;

    case TEXTURE_FORMAT_A16R16G16B16:
        *internalFormat = GL_RGBA;
        *format = GL_RGBA;
        *type = GL_UNSIGNED_SHORT;
        *compressed = false;
        success = true;
        break;

    case TEXTURE_FORMAT_A32R32G32B32:
        *internalFormat = GL_RGBA;
        *format = GL_RGBA;
        *type = GL_UNSIGNED_INT;
        *compressed = false;
        success = true;
        break;

    case TEXTURE_FORMAT_DXT1:
        *internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        *format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        *type = GL_UNSIGNED_BYTE;
        *compressed = true;
        success = true;
        break;

    case TEXTURE_FORMAT_DXT3:
        *internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        *format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        *type = GL_UNSIGNED_BYTE;
        *compressed = true;
        success = true;
        break;

    case TEXTURE_FORMAT_DXT5:
        *internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        *format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        *type = GL_UNSIGNED_BYTE;
        *compressed = true;
        success = true;
        break;

    case TEXTURE_FORMAT_PVRTC_4BPP_RGBA:
        *internalFormat = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
        *format = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
        *type = GL_UNSIGNED_BYTE;
        *compressed = true;
        success = true;
        break;

    case TEXTURE_FORMAT_PVRTC_2BPP_RGBA:
        *internalFormat = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
        *format = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
        *type = GL_UNSIGNED_BYTE;
        *compressed = true;
        success = true;
        break;

    case TEXTURE_FORMAT_PVRTC2_4BPP_RGBA:
        *internalFormat = GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG;
        *format = GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG;
        *type = GL_UNSIGNED_BYTE;
        *compressed = true;
        success = true;
        break;

    case TEXTURE_FORMAT_PVRTC2_2BPP_RGBA:
        *internalFormat = GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG;
        *format = GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG;
        *type = GL_UNSIGNED_BYTE;
        *compressed = true;
        success = true;
        break;

    case TEXTURE_FORMAT_ETC1:
        *internalFormat = GL_ETC1_RGB8_OES;
        *format = GL_ETC1_RGB8_OES;
        *type = GL_UNSIGNED_BYTE;
        *compressed = true;
        success = true;
        break;

    case TEXTURE_FORMAT_ETC2_R8G8B8:
        *internalFormat = GL_COMPRESSED_RGB8_ETC2;
        *format = GL_COMPRESSED_RGB8_ETC2;
        *type = GL_UNSIGNED_BYTE;
        *compressed = true;
        success = true;
        break;

    case TEXTURE_FORMAT_ETC2_R8G8B8A8:
        *internalFormat = GL_COMPRESSED_RGBA8_ETC2_EAC;
        *format = GL_COMPRESSED_RGBA8_ETC2_EAC;
        *type = GL_UNSIGNED_BYTE;
        *compressed = true;
        success = true;
        break;

    case TEXTURE_FORMAT_ETC2_R8G8B8A1:
        *internalFormat = GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
        *format = GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
        *type = GL_UNSIGNED_BYTE;
        *compressed = true;
        success = true;
        break;

    case TEXTURE_FORMAT_EAC_R11_UNSIGNED:
        *internalFormat = GL_COMPRESSED_R11_EAC;
        *format = GL_COMPRESSED_R11_EAC;
        *type = GL_UNSIGNED_BYTE;
        *compressed = true;
        success = true;
        break;

    case TEXTURE_FORMAT_EAC_R11_SIGNED:
        *internalFormat = GL_COMPRESSED_SIGNED_R11_EAC;
        *format = GL_COMPRESSED_SIGNED_R11_EAC;
        *type = GL_UNSIGNED_BYTE;
        *compressed = true;
        success = true;
        break;

    case TEXTURE_FORMAT_EAC_R11G11_UNSIGNED:
        *internalFormat = GL_COMPRESSED_RG11_EAC;
        *format = GL_COMPRESSED_RG11_EAC;
        *type = GL_UNSIGNED_BYTE;
        *compressed = true;
        success = true;
        break;

    case TEXTURE_FORMAT_EAC_R11G11_SIGNED:
        *internalFormat = GL_COMPRESSED_SIGNED_RG11_EAC;
        *format = GL_COMPRESSED_SIGNED_RG11_EAC;
        *type = GL_UNSIGNED_BYTE;
        *compressed = true;
        success = true;
        break;

    case TEXTURE_FORMAT_ATC_RGB:
        *internalFormat = GL_ATC_RGB_AMD;
        *format = GL_ATC_RGB_AMD;
        *type = GL_UNSIGNED_BYTE;
        *compressed = true;
        success = true;
        break;

    case TEXTURE_FORMAT_ATC_RGBA_EXPLICIT:
        *internalFormat = GL_ATC_RGBA_EXPLICIT_ALPHA_AMD;
        *format = GL_ATC_RGBA_EXPLICIT_ALPHA_AMD;
        *type = GL_UNSIGNED_BYTE;
        *compressed = true;
        success = true;
        break;

    case TEXTURE_FORMAT_ATC_RGBA_INTERPOLATED:
        *internalFormat = GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD;
        *format = GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD;
        *type = GL_UNSIGNED_BYTE;
        *compressed = true;
        success = true;
        break;

    case TEXTURE_FORMAT_R16F:
        *internalFormat = GL_R16F;
        *format = GL_RED;
        *type = GL_HALF_FLOAT;
        *compressed = false;
        success = true;
        break;

    case TEXTURE_FORMAT_RG16F:
        *internalFormat = GL_RG16F;
        *format = GL_RG;
        *type = GL_HALF_FLOAT;
        *compressed = false;
        success = true;
        break;

    case TEXTURE_FORMAT_RGBA16F:
        *internalFormat = GL_RGBA16F;
        *format = GL_RGBA;
        *type = GL_HALF_FLOAT;
        *compressed = false;
        success = true;
        break;

    case TEXTURE_FORMAT_R32F:
        *internalFormat = GL_R32F;
        *format = GL_RED;
        *type = GL_FLOAT;
        *compressed = false;
        success = true;
        break;

    case TEXTURE_FORMAT_RG32F:
        *internalFormat = GL_RG32F;
        *format = GL_RG;
        *type = GL_FLOAT;
        *compressed = false;
        success = true;
        break;

    case TEXTURE_FORMAT_RGBA32F:
        *internalFormat = GL_RGBA32F;
        *format = GL_RGBA;
        *type = GL_FLOAT;
        *compressed = false;
        success = true;
        break;

    default:
        success = false;
        DVASSERT(0, "Unsupported or unknown texture format specified");
    }

    return success;
}
