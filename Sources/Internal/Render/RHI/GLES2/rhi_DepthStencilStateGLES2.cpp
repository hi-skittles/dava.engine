#include "../Common/rhi_Private.h"
    #include "../Common/rhi_Pool.h"
    #include "rhi_GLES2.h"

    #include "Debug/DVAssert.h"
    #include "Logger/Logger.h"
using DAVA::Logger;

    #include "_gl.h"

namespace rhi
{
//==============================================================================

struct
DepthStencilStateGLES2_t
{
    uint32 depthTestEnabled : 1;
    GLuint depthMask;
    GLenum depthFunc;

    uint32 stencilEnabled : 1;
    uint32 stencilSeparate : 1;
    struct
    {
        GLenum failOp;
        GLenum depthFailOp;
        GLenum depthStencilPassOp;
        GLenum func;
        uint32 writeMask;
        uint32 readMask;
        uint32 refValue;
    } stencilFront, stencilBack;
};

typedef ResourcePool<DepthStencilStateGLES2_t, RESOURCE_DEPTHSTENCIL_STATE, DepthStencilState::Descriptor, false> DepthStencilStateGLES2Pool;
RHI_IMPL_POOL(DepthStencilStateGLES2_t, RESOURCE_DEPTHSTENCIL_STATE, DepthStencilState::Descriptor, false);

//------------------------------------------------------------------------------

static GLenum
_CmpFuncGLES2(CmpFunc func)
{
    GLenum f = GL_ALWAYS;

    switch (func)
    {
    case CMP_NEVER:
        f = GL_NEVER;
        break;
    case CMP_LESS:
        f = GL_LESS;
        break;
    case CMP_EQUAL:
        f = GL_EQUAL;
        break;
    case CMP_LESSEQUAL:
        f = GL_LEQUAL;
        break;
    case CMP_GREATER:
        f = GL_GREATER;
        break;
    case CMP_NOTEQUAL:
        f = GL_NOTEQUAL;
        break;
    case CMP_GREATEREQUAL:
        f = GL_GEQUAL;
        break;
    case CMP_ALWAYS:
        f = GL_ALWAYS;
        break;
    }

    return f;
}

//------------------------------------------------------------------------------

static GLenum
_StencilOpGLES2(StencilOperation op)
{
    GLenum s = GL_KEEP;

    switch (op)
    {
    case STENCILOP_KEEP:
        s = GL_KEEP;
        break;
    case STENCILOP_ZERO:
        s = GL_ZERO;
        break;
    case STENCILOP_REPLACE:
        s = GL_REPLACE;
        break;
    case STENCILOP_INVERT:
        s = GL_INVERT;
        break;
    case STENCILOP_INCREMENT_CLAMP:
        s = GL_INCR;
        break;
    case STENCILOP_DECREMENT_CLAMP:
        s = GL_DECR;
        break;
    case STENCILOP_INCREMENT_WRAP:
        s = GL_INCR_WRAP;
        break;
    case STENCILOP_DECREMENT_WRAP:
        s = GL_DECR_WRAP;
        break;
    }

    return s;
}

//==============================================================================

static Handle
gles2_DepthStencilState_Create(const DepthStencilState::Descriptor& desc)
{
    Handle handle = DepthStencilStateGLES2Pool::Alloc();
    DepthStencilStateGLES2_t* state = DepthStencilStateGLES2Pool::Get(handle);

    state->depthTestEnabled = desc.depthTestEnabled;
    state->depthMask = (desc.depthWriteEnabled) ? GL_TRUE : GL_FALSE;
    state->depthFunc = _CmpFuncGLES2(CmpFunc(desc.depthFunc));

    state->stencilEnabled = desc.stencilEnabled;
    state->stencilSeparate = desc.stencilTwoSided;

    state->stencilFront.failOp = _StencilOpGLES2(StencilOperation(desc.stencilFront.failOperation));
    state->stencilFront.depthFailOp = _StencilOpGLES2(StencilOperation(desc.stencilFront.depthFailOperation));
    state->stencilFront.depthStencilPassOp = _StencilOpGLES2(StencilOperation(desc.stencilFront.depthStencilPassOperation));
    state->stencilFront.func = _CmpFuncGLES2(CmpFunc(desc.stencilFront.func));
    state->stencilFront.readMask = desc.stencilFront.readMask;
    state->stencilFront.writeMask = desc.stencilFront.writeMask;
    state->stencilFront.refValue = desc.stencilFront.refValue;

    state->stencilBack.failOp = _StencilOpGLES2(StencilOperation(desc.stencilBack.failOperation));
    state->stencilBack.depthFailOp = _StencilOpGLES2(StencilOperation(desc.stencilBack.depthFailOperation));
    state->stencilBack.depthStencilPassOp = _StencilOpGLES2(StencilOperation(desc.stencilBack.depthStencilPassOperation));
    state->stencilBack.func = _CmpFuncGLES2(CmpFunc(desc.stencilBack.func));
    state->stencilBack.readMask = desc.stencilBack.readMask;
    state->stencilBack.writeMask = desc.stencilBack.writeMask;
    state->stencilBack.refValue = desc.stencilBack.refValue;

    return handle;
}

//------------------------------------------------------------------------------

void gles2_DepthStencilState_Delete(Handle state)
{
    DepthStencilStateGLES2Pool::Free(state);
}

//==============================================================================

namespace DepthStencilStateGLES2
{
int _GLES2_depthTestEnabled = -1;
int _GLES2_stencilEnabled = -1;
int _GLES2_depthMask = -1;
GLenum _GLES2_depthFunc = 0;

void Init(uint32 maxCount)
{
    DepthStencilStateGLES2Pool::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_DepthStencilState_Create = &gles2_DepthStencilState_Create;
    dispatch->impl_DepthStencilState_Delete = &gles2_DepthStencilState_Delete;
}

void InvalidateCache()
{
    _GLES2_depthTestEnabled = -1;
    _GLES2_stencilEnabled = -1;
    _GLES2_depthMask = -1;
    _GLES2_depthFunc = 0;
}

void SetToRHI(Handle hstate)
{
    DepthStencilStateGLES2_t* state = DepthStencilStateGLES2Pool::Get(hstate);

    if (state->depthTestEnabled)
    {
        if (_GLES2_depthTestEnabled != GL_TRUE)
        {
            GL_CALL(glEnable(GL_DEPTH_TEST));
            _GLES2_depthTestEnabled = GL_TRUE;
        }

        if (_GLES2_depthFunc != state->depthFunc)
        {
            GL_CALL(glDepthFunc(state->depthFunc));
            _GLES2_depthFunc = state->depthFunc;
        }
    }
    else
    {
        if (_GLES2_depthTestEnabled != GL_FALSE)
        {
            GL_CALL(glDisable(GL_DEPTH_TEST));
            _GLES2_depthTestEnabled = GL_FALSE;
        }
    }

    if (_GLES2_depthMask != state->depthMask)
    {
        GL_CALL(glDepthMask(state->depthMask));
        _GLES2_depthMask = state->depthMask;
    }

    if (state->stencilEnabled)
    {
        if (_GLES2_stencilEnabled != GL_TRUE)
        {
            GL_CALL(glEnable(GL_STENCIL_TEST));
            _GLES2_stencilEnabled = GL_TRUE;
        }

        if (state->stencilSeparate)
        {
            GL_CALL(glStencilOpSeparate(GL_FRONT, state->stencilFront.failOp, state->stencilFront.depthFailOp, state->stencilFront.depthStencilPassOp));
            GL_CALL(glStencilFuncSeparate(GL_FRONT, state->stencilFront.func, state->stencilFront.refValue, state->stencilFront.readMask));
            GL_CALL(glStencilMaskSeparate(GL_FRONT, state->stencilFront.writeMask));

            GL_CALL(glStencilOpSeparate(GL_BACK, state->stencilBack.failOp, state->stencilBack.depthFailOp, state->stencilBack.depthStencilPassOp));
            GL_CALL(glStencilFuncSeparate(GL_BACK, state->stencilBack.func, state->stencilBack.refValue, state->stencilBack.readMask));
            GL_CALL(glStencilMaskSeparate(GL_BACK, state->stencilBack.writeMask));
        }
        else
        {
            GL_CALL(glStencilOp(state->stencilFront.failOp, state->stencilFront.depthFailOp, state->stencilFront.depthStencilPassOp));
            GL_CALL(glStencilFunc(state->stencilFront.func, state->stencilFront.refValue, state->stencilFront.readMask));
            GL_CALL(glStencilMask(state->stencilFront.writeMask));
        }
    }
    else
    {
        if (_GLES2_stencilEnabled != GL_FALSE)
        {
            GL_CALL(glDisable(GL_STENCIL_TEST));
            _GLES2_stencilEnabled = GL_FALSE;
        }
    }
}
}

//==============================================================================
} // namespace rhi
