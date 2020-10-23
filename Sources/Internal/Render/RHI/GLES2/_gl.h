#pragma once

#include "../rhi_Type.h"

#if defined(__DAVAENGINE_WIN32__)

#include "GL/glew.h"
#include <GL/GL.h>
#include "GL/wglew.h"


    
#define GetGLErrorString gluErrorString

#include "win_gl.h"

#elif defined(__DAVAENGINE_MACOS__)

    #include <Carbon/Carbon.h>
    #include <AGL/agl.h>
    #include <OpenGL/gl3.h>
    #include <OpenGL/gl3ext.h>

    #define GetGLErrorString(code) #code

    #include "macos_gl.h"

#elif defined(__DAVAENGINE_IPHONE__)

    #include <OpenGLES/ES3/gl.h>
    #include <OpenGLES/ES3/glext.h>
    #include <OpenGLES/ES2/glext.h>

    #define GetGLErrorString(code) "<unknown>"

    #include "ios_gl.h"

#elif defined(__DAVAENGINE_ANDROID__)

	#include <EGL/egl.h>
	#include <GLES2/gl2.h>
	#include <GLES2/gl2ext.h>

    #define GetGLErrorString(code) "<unknown>"

	#include "android_gl.h"

#ifdef __arm__
    #include <machine/cpu-features.h>
    #if __ARM_ARCH__ == 7
        #ifdef USE_NEON
            #define __DAVAENGINE_ARM_7__
        #endif
    #endif
#endif

#else

    #include <GL/GL.h>

#endif

#if defined(__DAVAENGINE_ANDROID__)

//Valid for clang. For MSVC and GCC instead '#func postfix' should be '#func##postfix'
#define GET_GL_FUNC(func, postfix)                                                 \
{                                                                                  \
    func = reinterpret_cast<decltype(func)>(eglGetProcAddress(#func));             \
    if (func == nullptr)                                                            \
        func = reinterpret_cast<decltype(func)>(eglGetProcAddress(#func postfix)); \
}

typedef DAVA::uint64 GLuint64;

typedef void(GL_APIENTRY* PFNGLEGL_GLDRAWELEMENTSINSTANCED)(GLenum, GLsizei, GLenum, const void*, GLsizei);
typedef void(GL_APIENTRY* PFNGLEGL_GLDRAWARRAYSINSTANCED)(GLenum, GLint, GLsizei, GLsizei);
typedef void(GL_APIENTRY* PFNGLEGL_GLVERTEXATTRIBDIVISOR)(GLuint, GLuint);
typedef void(GL_APIENTRY* PFNGLEGL_GLRENDERBUFFERSTORAGEMULTISAMPLE)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
typedef void(GL_APIENTRY* PFNGLEGL_GLBLITFRAMEBUFFERANGLEPROC)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

typedef void(GL_APIENTRY* PFNGLGENQUERIESEXTPROC)(GLsizei n, GLuint* ids);
typedef void(GL_APIENTRY* PFNGLDELETEQUERIESEXTPROC)(GLsizei n, const GLuint* ids);
typedef void(GL_APIENTRY* PFNGLBEGINQUERYEXTPROC)(GLenum target, GLuint id);
typedef void(GL_APIENTRY* PFNGLENDQUERYEXTPROC)(GLenum target);
typedef void(GL_APIENTRY* PFNGLQUERYCOUNTEREXTPROC)(GLuint id, GLenum target);
typedef void(GL_APIENTRY* PFNGLGETQUERYOBJECTUIVEXTPROC)(GLuint id, GLenum pname, GLuint* params);
typedef void(GL_APIENTRY* PFNGLGETQUERYOBJECTUI64VEXTPROC)(GLuint id, GLenum pname, GLuint64* params);

// GL_KHR_debug
typedef void(GL_APIENTRY* GLDEBUGPROCKHR)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
typedef void(GL_APIENTRY* PFNGL_DEBUGMESSAGECONTROLKHRPROC)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint* ids, GLboolean enabled);
typedef void(GL_APIENTRY* PFNGL_DEBUGMESSAGECALLBACKKHRPROC)(GLDEBUGPROCKHR callback, const void* userParam);

extern PFNGLEGL_GLDRAWELEMENTSINSTANCED glDrawElementsInstanced;
extern PFNGLEGL_GLDRAWARRAYSINSTANCED glDrawArraysInstanced;
extern PFNGLEGL_GLVERTEXATTRIBDIVISOR glVertexAttribDivisor;
extern PFNGLEGL_GLRENDERBUFFERSTORAGEMULTISAMPLE glRenderbufferStorageMultisample;
extern PFNGLEGL_GLBLITFRAMEBUFFERANGLEPROC glBlitFramebuffer;
extern PFNGL_DEBUGMESSAGECONTROLKHRPROC glDebugMessageControl;
extern PFNGL_DEBUGMESSAGECALLBACKKHRPROC glDebugMessageCallback;

extern PFNGLGENQUERIESEXTPROC glGenQueries;
extern PFNGLDELETEQUERIESEXTPROC glDeleteQueries;
extern PFNGLBEGINQUERYEXTPROC glBeginQuery;
extern PFNGLENDQUERYEXTPROC glEndQuery;
extern PFNGLQUERYCOUNTEREXTPROC glQueryCounter;
extern PFNGLGETQUERYOBJECTUIVEXTPROC glGetQueryObjectuiv;
extern PFNGLGETQUERYOBJECTUI64VEXTPROC glGetQueryObjectui64v;

#endif

#if defined(__DAVAENGINE_ANDROID__) || defined(__DAVAENGINE_WIN_UAP__)
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif //GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif //GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#endif //defined(__DAVAENGINE_ANDROID__) || defined (__DAVAENGINE_WIN_UAP__)

#if !defined(GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG)
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
#endif
#if !defined(GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG)
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 0x8C03
#endif

#if !defined(GL_COMPRESSED_RGB_S3TC_DXT1_EXT)
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#endif

#if !defined(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif

#if !defined(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif

#if !defined(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

#if !defined(GL_ETC1_RGB8_OES)
#define GL_ETC1_RGB8_OES 0x8D64
#endif

#if !defined(GL_ATC_RGB_AMD)
#define GL_ATC_RGB_AMD 0x8C92
#endif

#if !defined(GL_ATC_RGBA_EXPLICIT_ALPHA_AMD)
#define GL_ATC_RGBA_EXPLICIT_ALPHA_AMD 0x8C93
#endif

#if !defined(GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD)
#define GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD 0x87EE
#endif

#if !defined(GL_COMPRESSED_R11_EAC)
#define GL_COMPRESSED_R11_EAC 0x9270
#endif

#if !defined(GL_COMPRESSED_SIGNED_R11_EAC)
#define GL_COMPRESSED_SIGNED_R11_EAC 0x9271
#endif

#if !defined(GL_COMPRESSED_RG11_EAC)
#define GL_COMPRESSED_RG11_EAC 0x9272
#endif

#if !defined(GL_COMPRESSED_SIGNED_RG11_EAC)
#define GL_COMPRESSED_SIGNED_RG11_EAC 0x9273
#endif

#if !defined(GL_COMPRESSED_RGB8_ETC2)
#define GL_COMPRESSED_RGB8_ETC2 0x9274
#endif

#if !defined(GL_COMPRESSED_RGBA8_ETC2_EAC)
#define GL_COMPRESSED_RGBA8_ETC2_EAC 0x9278
#endif

#if !defined(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2)
#define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
#endif

#if !defined(GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG)
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG 0x9137
#endif

#if !defined(GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG)
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG 0x9138
#endif

#if !defined(GL_HALF_FLOAT_OES)
#define GL_HALF_FLOAT_OES 0x8D61
#endif

#if !defined GL_QUERY_RESULT_AVAILABLE_EXT
#define GL_QUERY_RESULT_AVAILABLE_EXT 0x8867
#endif

#if !defined GL_QUERY_RESULT_EXT
#define GL_QUERY_RESULT_EXT 0x8866
#endif

#if !defined(GL_HALF_FLOAT)
#define GL_HALF_FLOAT GL_HALF_FLOAT_OES
#endif

#if !defined(GL_DEPTH_COMPONENT24)
#define GL_DEPTH_COMPONENT24 GL_DEPTH_COMPONENT24_OES
#endif

#if !defined(GL_BGRA)
#define GL_BGRA GL_BGRA_EXT
#endif

#if !defined(GL_DEPTH24_STENCIL8)
#define GL_DEPTH24_STENCIL8 GL_DEPTH24_STENCIL8_OES
#endif

#if !defined(GL_QUERY_RESULT_AVAILABLE)
#define GL_QUERY_RESULT_AVAILABLE GL_QUERY_RESULT_AVAILABLE_EXT
#endif

#if !defined(GL_QUERY_RESULT)
#define GL_QUERY_RESULT GL_QUERY_RESULT_EXT
#endif

#if !defined(GL_RED)
#define GL_RED 0x1903
#endif

#if !defined(GL_RG)
#define GL_RG 0x8227
#endif

#ifndef GL_RGB565
#define GL_RGB565 0x8D62
#endif

#if !defined(GL_R8)
#define GL_R8 0x8229
#endif

#if !defined(GL_R16)
#define GL_R16 0x822A
#endif

#if !defined(GL_RG8)
#define GL_RG8 0x822B
#endif

#if !defined(GL_RG16)
#define GL_RG16 0x822C
#endif

#if !defined(GL_R16F)
#define GL_R16F 0x822D
#endif

#if !defined(GL_R32F)
#define GL_R32F 0x822E
#endif

#if !defined(GL_RG16F)
#define GL_RG16F 0x822F
#endif

#if !defined(GL_RG32F)
#define GL_RG32F 0x8230
#endif

#if !defined(GL_RGBA32F) && defined(GL_RGBA32F_ARB)
    #define GL_RGBA32F GL_RGBA32F_ARB
#endif

#if !defined(GL_RGBA32F) && defined(GL_RGBA32F_EXT)
    #define GL_RGBA32F GL_RGBA32F_EXT
#endif

#if !defined(GL_RGBA32F)
    #define GL_RGBA32F 0x8814
#endif

#if !defined(GL_RGB32F)
#define GL_RGB32F 0x8815
#endif

#if !defined(GL_RGBA16F) && defined(GL_RGBA16F_ARB)
    #define GL_RGBA16F GL_RGBA16F_ARB
#endif

#if !defined(GL_RGBA16F) && defined(GL_RGBA16F_EXT)
    #define GL_RGBA16F GL_RGBA16F_EXT
#endif

#if !defined(GL_RGBA16F)
    #define GL_RGBA16F 0x881A
#endif

#if !defined(GL_RGB16F)
    #define GL_RGB16F 0x881B
#endif

#if !defined(GL_RGBA8)
    #if defined(GL_RGBA8_OES)
        #define GL_RGBA8 GL_RGBA8_OES
    #else
        #define GL_RGBA GL_RGBA
    #endif
#endif

#if !defined(GL_READ_FRAMEBUFFER)
    #define GL_READ_FRAMEBUFFER 0x8CA8
#endif

#if !defined(GL_DRAW_FRAMEBUFFER)
    #define GL_DRAW_FRAMEBUFFER 0x8CA9
#endif

#if !defined(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT)
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

#if !defined(GL_TEXTURE_MAX_ANISOTROPY_EXT)
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif

#if !defined(GL_TIME_ELAPSED)
#define GL_TIME_ELAPSED 0x88BF
#endif

#if !defined(GL_TIMESTAMP)
#define GL_TIMESTAMP 0x8E28
#endif

#if !defined(GL_GPU_DISJOINT)
#define GL_GPU_DISJOINT 0x8FBB
#endif

#if !defined(GL_MAX_SAMPLES)
#define GL_MAX_SAMPLES 0x8D57
#endif

#if !defined(GL_DEBUG_OUTPUT) && defined(GL_DEBUG_OUTPUT_KHR)
    #define GL_DEBUG_OUTPUT GL_DEBUG_OUTPUT_KHR
#endif

#if !defined(GL_DEBUG_OUTPUT)
    #define GL_DEBUG_OUTPUT 0x92E0
#endif

#if defined(__DAVAENGINE_ANDROID__) && defined(__DAVAENGINE_ARM_7__)

extern volatile struct alignas(32) GLCallRegisters
{
    DAVA::uint8 registers[64];
} gl_call_registers;

#endif

#if 0
#define GL_CALL(expr) \
{ \
    expr; \
    GLint err = glGetError(); \
    if (err != GL_NO_ERROR) \
    { \
        DAVA::Logger::Error("OpenGL call %s failed with %s", #expr, glErrorToString(err)); \
    } \
}

#else

#if defined(__DAVAENGINE_ANDROID__) && defined(__DAVAENGINE_ARM_7__)

#define GL_CALL(expr) \
{ \
    if (_GLES2_ValidateNeonCalleeSavedRegisters) \
    { \
        asm volatile("vstmia %0, {q4-q7}" ::"r"(gl_call_registers.registers) \
                         : "memory"); \
        expr; \
        asm volatile("vldmia %0, {q4-q7}" ::"r"(gl_call_registers.registers) \
                         : "q4", "q5", "q6", "q7"); \
    } \
    else \
    { \
        expr; \
    }\
}

#else

#define GL_CALL(expr) expr;

#endif

#endif

extern GLuint _GLES2_Bound_FrameBuffer;
extern GLuint _GLES2_Default_FrameBuffer;
extern void* _GLES2_Native_Window;
extern void* _GLES2_Context;
extern void (*_GLES2_AcquireContext)();
extern void (*_GLES2_ReleaseContext)();

extern int _GLES2_DefaultFrameBuffer_Width;
extern int _GLES2_DefaultFrameBuffer_Height;

extern GLuint _GLES2_LastSetIB;
extern DAVA::uint8* _GLES2_LastSetIndices;
extern GLuint _GLES2_LastSetVB;
extern GLuint _GLES2_LastSetTex0;
extern GLenum _GLES2_LastSetTex0Target;
extern int _GLES2_LastActiveTexture;

extern bool _GLES2_IsDebugSupported;
extern bool _GLES2_IsGlDepth24Stencil8Supported;
extern bool _GLES2_IsGlDepthNvNonLinearSupported;
extern bool _GLES2_UseUserProvidedIndices;
extern bool _GLES2_TimeStampQuerySupported;
extern volatile bool _GLES2_ValidateNeonCalleeSavedRegisters;

bool GetGLTextureFormat(rhi::TextureFormat rhiFormat, GLint* internalFormat, GLint* format, GLenum* type, bool* compressed);
GLint GetGLRenderTargetFormat(rhi::TextureFormat rhiFormat);

const char* glErrorToString(GLint error);
