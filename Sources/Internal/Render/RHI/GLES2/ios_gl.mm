#include "_gl.h"

#if defined(__DAVAENGINE_IPHONE__)

#import <QuartzCore/QuartzCore.h>

#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

#import <UIKit/UIKit.h>

static GLuint colorRenderbuffer = -1;
static GLuint depthRenderbuffer = -1;
static GLint backingWidth = 0;
static GLint backingHeight = 0;
static bool resize_pending = true;
static EAGLRenderingAPI renderingAPI = kEAGLRenderingAPIOpenGLES2;

//------------------------------------------------------------------------------

bool ios_gl_check_layer()
{
    if (!resize_pending)
        return YES;

    // Allocate color buffer backing based on the current layer size
    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
    [(EAGLContext*)_GLES2_Context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer*)_GLES2_Native_Window];
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight);

    _GLES2_DefaultFrameBuffer_Width = backingWidth;
    _GLES2_DefaultFrameBuffer_Height = backingHeight;

    if (depthRenderbuffer != GLuint(-1))
    {
        glDeleteRenderbuffers(1, &depthRenderbuffer);
    }

    glGenRenderbuffers(1, &depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, backingWidth, backingHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
    }
    else
    {
        resize_pending = false;
    }

    return NO;
}

void ios_gl_init(void* nativeLayer)
{
    _GLES2_Native_Window = nativeLayer;

    renderingAPI = kEAGLRenderingAPIOpenGLES3;
    _GLES2_Context = [[EAGLContext alloc] initWithAPI:renderingAPI];
    if (_GLES2_Context == nil)
    {
        renderingAPI = kEAGLRenderingAPIOpenGLES2;
        _GLES2_Context = [[EAGLContext alloc] initWithAPI:renderingAPI];
    }

    [EAGLContext setCurrentContext:(EAGLContext*)_GLES2_Context];

    glGenFramebuffers(1, &_GLES2_Default_FrameBuffer);
    glGenRenderbuffers(1, &colorRenderbuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, _GLES2_Default_FrameBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer);

    _GLES2_Bound_FrameBuffer = _GLES2_Default_FrameBuffer;

    ios_gl_check_layer();
}

void ios_gl_begin_frame()
{
}

void ios_gl_reset(void* nativeLayer, GLint width, GLint height)
{
    resize_pending = (width != backingWidth) || (height != backingHeight) || (_GLES2_Native_Window != nativeLayer);

    _GLES2_Native_Window = nativeLayer;
}

void ios_gl_end_frame()
{
    glBindFramebuffer(GL_FRAMEBUFFER, _GLES2_Default_FrameBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);

    ios_gl_discard_framebuffer(false, true);

    [(EAGLContext*)_GLES2_Context presentRenderbuffer:GL_RENDERBUFFER];
}

void ios_gl_acquire_context()
{
    [EAGLContext setCurrentContext:(EAGLContext*)_GLES2_Context];
}

void ios_gl_release_context()
{
    [EAGLContext setCurrentContext:nullptr];
}

void ios_gl_resolve_multisampling(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                                  GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
    if (renderingAPI == kEAGLRenderingAPIOpenGLES2)
    {
        GL_CALL(glResolveMultisampleFramebufferAPPLE());
    }
    else
    {
        GL_CALL(glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter));
    }
}

void ios_gl_discard_framebuffer(bool discardColor, bool discardDepthStencil)
{
    DVASSERT(discardColor || discardDepthStencil);

    GLenum discards[3] = {};
    GLsizei discardCount = 0;

    if (discardColor)
    {
        discards[discardCount++] = GL_COLOR_ATTACHMENT0;
    }

    if (discardDepthStencil)
    {
        discards[discardCount++] = GL_DEPTH_ATTACHMENT;
        discards[discardCount++] = GL_STENCIL_ATTACHMENT;
    }

    if (renderingAPI == kEAGLRenderingAPIOpenGLES2)
    {
        GL_CALL(glDiscardFramebufferEXT(GL_FRAMEBUFFER, discardCount, discards));
    }
    else
    {
        GL_CALL(glInvalidateFramebuffer(GL_FRAMEBUFFER, discardCount, discards));
    }
}

#endif
