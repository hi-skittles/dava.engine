#include "Base/Platform.h"
#include "Logger/Logger.h"
#include "Debug/DVAssert.h"
#include "Concurrency/Mutex.h"
#include "Concurrency/LockGuard.h"

#ifdef __DAVAENGINE_ANDROID__

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <android/native_window.h>

#include "android_gl.h"
#include "_gl.h"

static EGLDisplay _display = EGL_NO_DISPLAY;
static EGLSurface _surface = EGL_NO_SURFACE;
static EGLContext _context = EGL_NO_CONTEXT;
static EGLint _format = 0;
static EGLConfig _config = 0;

static ANativeWindow* _nativeWindow = nullptr;
static GLint backingWidth = 0;
static GLint backingHeight = 0;
static bool needRecreateSurface = false;
static DAVA::Mutex surfaceMutex;
static bool invokedResetWithResize = false;

PFNGLEGL_GLDRAWELEMENTSINSTANCED glDrawElementsInstanced = nullptr;
PFNGLEGL_GLDRAWARRAYSINSTANCED glDrawArraysInstanced = nullptr;
PFNGLEGL_GLVERTEXATTRIBDIVISOR glVertexAttribDivisor = nullptr;
PFNGLEGL_GLBLITFRAMEBUFFERANGLEPROC glBlitFramebuffer = nullptr;
PFNGLEGL_GLRENDERBUFFERSTORAGEMULTISAMPLE glRenderbufferStorageMultisample = nullptr;
PFNGL_DEBUGMESSAGECONTROLKHRPROC glDebugMessageControl;
PFNGL_DEBUGMESSAGECALLBACKKHRPROC glDebugMessageCallback;

PFNGLGENQUERIESEXTPROC glGenQueries = nullptr;
PFNGLDELETEQUERIESEXTPROC glDeleteQueries = nullptr;
PFNGLBEGINQUERYEXTPROC glBeginQuery = nullptr;
PFNGLENDQUERYEXTPROC glEndQuery = nullptr;
PFNGLQUERYCOUNTEREXTPROC glQueryCounter = nullptr;
PFNGLGETQUERYOBJECTUIVEXTPROC glGetQueryObjectuiv = nullptr;
PFNGLGETQUERYOBJECTUI64VEXTPROC glGetQueryObjectui64v = nullptr;

static const EGLint contextAttribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};

void android_gl_init(void* _window)
{
    _nativeWindow = static_cast<ANativeWindow*>(_window);

    const EGLint d24s8ConfigAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_BUFFER_SIZE, 32,
        EGL_DEPTH_SIZE, 24,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };

    const EGLint d16s8NvidiaConfigAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_BUFFER_SIZE, 32,
        EGL_DEPTH_SIZE, 16,
        EGL_STENCIL_SIZE, 8,
        EGL_DEPTH_ENCODING_NV, EGL_DEPTH_ENCODING_NONLINEAR_NV,
        EGL_NONE
    };

    const EGLint d16s8ConfigAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_BUFFER_SIZE, 32,
        EGL_DEPTH_SIZE, 16,
        EGL_STENCIL_SIZE, 8,
        EGL_NONE
    };

    EGLint numConfigs;

    _display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(_display, nullptr, nullptr);

    //try initialize 24 bit depth buffer
    eglChooseConfig(_display, d24s8ConfigAttribs, &_config, 1, &numConfigs);
    if (_config == nullptr)
    {
        //try initialize 16 bit depth buffer with NVidia extension
        eglChooseConfig(_display, d16s8NvidiaConfigAttribs, &_config, 1, &numConfigs);
    }
    if (_config == nullptr)
    {
        //worst case only 16 bit depth buffer
        eglChooseConfig(_display, d16s8ConfigAttribs, &_config, 1, &numConfigs);
    }
    DVASSERT(_config != nullptr, "Can't set GL configuration");

    eglGetConfigAttrib(_display, _config, EGL_NATIVE_VISUAL_ID, &_format);

    backingWidth = _GLES2_DefaultFrameBuffer_Width;
    backingHeight = _GLES2_DefaultFrameBuffer_Height;

    ANativeWindow_setBuffersGeometry(_nativeWindow, _GLES2_DefaultFrameBuffer_Width, _GLES2_DefaultFrameBuffer_Height, _format);
    _surface = eglCreateWindowSurface(_display, _config, _nativeWindow, nullptr);

    _context = eglCreateContext(_display, _config, EGL_NO_CONTEXT, contextAttribs);
    _GLES2_Context = _context;

    eglMakeCurrent(_display, _surface, _surface, _context);
}

void android_gl_reset(void* _window, GLint width, GLint height)
{
    DAVA::LockGuard<DAVA::Mutex> guard(surfaceMutex);

    ANativeWindow* nativeWindow = static_cast<ANativeWindow*>(_window);
    if (nullptr != nativeWindow)
    {
        invokedResetWithResize = (backingWidth != width) || (backingHeight != height);
        if (_nativeWindow != nativeWindow || invokedResetWithResize)
        {
            needRecreateSurface = true;
            backingWidth = width;
            backingHeight = height;
        }
    }
    _nativeWindow = nativeWindow;
}

bool android_gl_checkSurface()
{
    DAVA::LockGuard<DAVA::Mutex> guard(surfaceMutex);

    if (_nativeWindow == nullptr)
    {
        return false;
    }

    if (needRecreateSurface)
    {
        // Why this should work I do not fully understand, but this solution works
        // For more info see SDL sources: SDL2-2.0.4\src\core\android\SDL_android.c, Java_org_libsdl_app_SDLActivity_onNativeSurfaceDestroyed function
        // Also see http://stackoverflow.com/questions/8762589/eglcreatewindowsurface-on-ics-and-switching-from-2d-to-3d
        eglMakeCurrent(_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(_display, _surface);

        _GLES2_DefaultFrameBuffer_Width = backingWidth;
        _GLES2_DefaultFrameBuffer_Height = backingHeight;

        ANativeWindow_setBuffersGeometry(_nativeWindow, _GLES2_DefaultFrameBuffer_Width, _GLES2_DefaultFrameBuffer_Height, _format);

        _surface = eglCreateWindowSurface(_display, _config, _nativeWindow, nullptr);
        eglMakeCurrent(_display, _surface, _surface, _context);

        needRecreateSurface = false;

        // Next frame should be rejected only if we called reset with size different from a previous one
        if (invokedResetWithResize)
        {
            return false;
        }
    }

    return true;
}

bool android_gl_end_frame()
{
    EGLBoolean ret = eglSwapBuffers(_display, _surface);

    if (!ret && eglGetError() == EGL_CONTEXT_LOST)
    {
        DAVA::Logger::Error("Context Lost");
        eglDestroyContext(_display, _context);
        _GLES2_Context = _context = eglCreateContext(_display, _config, EGL_NO_CONTEXT, contextAttribs);

        eglMakeCurrent(_display, _surface, _surface, _context);

        return false; //if context was lost, return 'false' (need recreate all resources)
    }

    return true;
}

void android_gl_acquire_context()
{
    eglMakeCurrent(_display, _surface, _surface, _context);
}

void android_gl_release_context()
{
    eglMakeCurrent(_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

#endif
