#include "../Common/rhi_Private.h"
#include "../Common/rhi_Pool.h"
#include "../Common/rhi_Utils.h"
#include "../Common/rhi_FormatConversion.h"
#include "../rhi_Public.h"
#include "rhi_GLES2.h"
#include "Debug/DVAssert.h"
#include "Logger/Logger.h"
#include "_gl.h"

namespace rhi
{
//==============================================================================

struct FramebufferGLES2_t
{
    Handle color[MAX_RENDER_TARGET_COUNT];
    GLuint colorUID[MAX_RENDER_TARGET_COUNT];
    TextureFace colorFace[MAX_RENDER_TARGET_COUNT];
    unsigned colorLevel[MAX_RENDER_TARGET_COUNT];
    uint32 colorCount = 0;
    Handle depthStencil = InvalidHandle;
    GLuint depthStencilUID = 0;
    TextureFace face = TextureFace::TEXTURE_FACE_NONE;
    uint32_t level = 0;
    GLuint frameBuffer = 0;
};
static std::vector<FramebufferGLES2_t> TextureFBO;

class TextureGLES2_t : public ResourceImpl<TextureGLES2_t, Texture::Descriptor>
{
public:
    TextureGLES2_t();

    bool Create(const Texture::Descriptor& desc, bool forceExecute = false);
    void Destroy(bool forceExecute);

    uint32 uid = 0;
    uint32 uid2 = 0;
    uint32 width = 0;
    uint32 height = 0;
    uint32 sampleCount = 1;
    TextureFormat format = TextureFormat::TEXTURE_FORMAT_R8G8B8A8;
    uint32 mappedLevel = 0;
    GLenum mappedFace = 0;

    void* mappedData = nullptr;
    SamplerState::Descriptor::Sampler samplerState;
    GLint fbo = 0;

    uint32 isMapped : 1;
    uint32 updatePending : 1;
    uint32 isCubeMap : 1;
    uint32 isRenderTarget : 1;
    uint32 isRenderBuffer : 1;
    uint32 forceSetSamplerState : 1;
};

RHI_IMPL_RESOURCE(TextureGLES2_t, Texture::Descriptor)

typedef ResourcePool<TextureGLES2_t, RESOURCE_TEXTURE, Texture::Descriptor, true> TextureGLES2Pool;
RHI_IMPL_POOL(TextureGLES2_t, RESOURCE_TEXTURE, Texture::Descriptor, true);

TextureGLES2_t::TextureGLES2_t()
    : isMapped(false)
    , updatePending(false)
    , isCubeMap(false)
    , isRenderTarget(false)
    , isRenderBuffer(false)
    , forceSetSamplerState(true)
{
}

//------------------------------------------------------------------------------

bool TextureGLES2_t::Create(const Texture::Descriptor& desc, bool forceExecute)
{
    DVASSERT(desc.levelCount);

    bool success = false;
    UpdateCreationDesc(desc);

    GLuint glObjects[2] = { 0, 0 };

    bool is_render_buffer = false;

    fbo = 0;

    if ((desc.format == TEXTURE_FORMAT_D16) || (desc.format == TEXTURE_FORMAT_D24S8))
    {
        is_render_buffer = true;
        GLCommand cmd1 = { GLCommand::GEN_RENDERBUFFERS, { static_cast<uint64>(_GLES2_IsGlDepth24Stencil8Supported ? 1 : 2),
                                                           reinterpret_cast<uint64>(glObjects) } };

        ExecGL(&cmd1, 1);

        if (cmd1.status == GL_NO_ERROR)
        {
            if (_GLES2_IsGlDepth24Stencil8Supported)
            {
                GLCommand d24s8cmd[] =
                {
                  { GLCommand::BIND_RENDERBUFFER, { GL_RENDERBUFFER, glObjects[0] } },
                  { GLCommand::RENDERBUFFER_STORAGE, { GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, desc.width, desc.height, desc.sampleCount } },
                  { GLCommand::BIND_RENDERBUFFER, { GL_RENDERBUFFER, 0 } }
                };
                ExecGL(d24s8cmd, countof(d24s8cmd), forceExecute);

                // Store depth/stencil buffer index as secondary stencil index for iOS/Android
                glObjects[1] = glObjects[0];
            }
            else
            {
                GLuint depthComponentFormat = GL_DEPTH_COMPONENT16;
			#if defined(__DAVAENGINE_ANDROID__)
                if (_GLES2_IsGlDepthNvNonLinearSupported)
                {
                    depthComponentFormat = GL_DEPTH_COMPONENT16_NONLINEAR_NV;
                }
			#endif
                GLCommand d16s8cmd[] =
                {
                  { GLCommand::BIND_RENDERBUFFER, { GL_RENDERBUFFER, glObjects[0] } },
                  { GLCommand::RENDERBUFFER_STORAGE, { GL_RENDERBUFFER, depthComponentFormat, desc.width, desc.height, desc.sampleCount } },

                  { GLCommand::BIND_RENDERBUFFER, { GL_RENDERBUFFER, glObjects[1] } },
                  { GLCommand::RENDERBUFFER_STORAGE, { GL_RENDERBUFFER, GL_STENCIL_INDEX8, desc.width, desc.height, desc.sampleCount } },

                  { GLCommand::BIND_RENDERBUFFER, { GL_RENDERBUFFER, 0 } }
                };
                ExecGL(d16s8cmd, countof(d16s8cmd), forceExecute);
            }
        }
    }
    else if (desc.sampleCount > 1) // create multisampled render target texture
    {
        DVASSERT(desc.isRenderTarget); // do not support plain multisampled textures
        DVASSERT(desc.type == rhi::TextureType::TEXTURE_TYPE_2D); // do not support cube maps now

        GLint int_fmt = GetGLRenderTargetFormat(desc.format);

        GLCommand gen = { GLCommand::GEN_RENDERBUFFERS, { 1, reinterpret_cast<uint64>(glObjects) } };
        ExecGL(&gen, 1, forceExecute);

        is_render_buffer = true;
        GLCommand cmd[] =
        {
          { GLCommand::BIND_RENDERBUFFER, { GL_RENDERBUFFER, glObjects[0] } },
          { GLCommand::RENDERBUFFER_STORAGE, { GL_RENDERBUFFER, static_cast<uint64>(int_fmt), desc.width, desc.height, desc.sampleCount } },
          { GLCommand::BIND_RENDERBUFFER, { GL_RENDERBUFFER, 0 } }
        };

        ExecGL(cmd, countof(cmd), forceExecute);
    }
    else // create plain texture
    {
        GLenum target = (desc.type == TEXTURE_TYPE_CUBE) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
        uint32 cmd2_cnt = 2;
        GLCommand cmd2[4 + countof(desc.initialData)] =
        {
          { GLCommand::GEN_TEXTURES, { 1, reinterpret_cast<uint64>(glObjects) } },
          { GLCommand::SET_ACTIVE_TEXTURE, { GL_TEXTURE0 + 0 } }
        };

        if (desc.autoGenMipmaps && !desc.isRenderTarget)
        {
            cmd2[3].func = GLCommand::GENERATE_MIPMAP;
            ++cmd2_cnt;
        }

        // process initial-data, if any
        {
            uint32 array_sz = (desc.type == TEXTURE_TYPE_CUBE) ? 6 : 1;
            GLenum face[] = { GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };
            GLint int_fmt = 0;
            GLint fmt = 0;
            GLenum type = 0;
            bool compressed = false;
            GetGLTextureFormat(desc.format, &int_fmt, &fmt, &type, &compressed);

            DVASSERT(desc.levelCount <= countof(desc.initialData));
            for (unsigned s = 0; s != array_sz; ++s)
            {
                cmd2[cmd2_cnt].func = GLCommand::BIND_TEXTURE;
                cmd2[cmd2_cnt].arg[0] = (desc.type == TEXTURE_TYPE_CUBE) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
                cmd2[cmd2_cnt].arg[1] = uint64(glObjects);
                ++cmd2_cnt;

                target = (desc.type == TEXTURE_TYPE_CUBE) ? face[s] : GL_TEXTURE_2D;

                for (unsigned m = 0; m != desc.levelCount; ++m)
                {
                    GLCommand* cmd = cmd2 + cmd2_cnt;
                    void* data = desc.initialData[s * desc.levelCount + m];

                    if (data)
                    {
                        Size2i sz = TextureExtents(Size2i(desc.width, desc.height), m);
                        uint32 data_sz = TextureSize(desc.format, sz.dx, sz.dy);

                        cmd->func = GLCommand::TEX_IMAGE2D;
                        cmd->arg[0] = target;
                        cmd->arg[1] = m;
                        cmd->arg[2] = uint64(int_fmt);
                        cmd->arg[3] = uint64(sz.dx);
                        cmd->arg[4] = uint64(sz.dy);
                        cmd->arg[5] = 0;
                        cmd->arg[6] = uint64(fmt);
                        cmd->arg[7] = type;
                        cmd->arg[8] = uint64(data_sz);
                        cmd->arg[9] = reinterpret_cast<uint64>(data);
                        cmd->arg[10] = compressed;

                        if (desc.format == TEXTURE_FORMAT_R4G4B4A4)
                            _FlipRGBA4_ABGR4(data, data, data_sz);
                        else if (desc.format == TEXTURE_FORMAT_R5G5B5A1)
                            _RGBA5551toABGR1555(data, data, data_sz);

                        ++cmd2_cnt;
                    }
                    else
                    {
                        break;
                    }
                }
            }

            cmd2[cmd2_cnt].func = GLCommand::RESTORE_TEXTURE0;
            ++cmd2_cnt;
        }

        ExecGL(cmd2, cmd2_cnt, forceExecute);
    }

    if (glObjects[0])
    {
        uid = glObjects[0];
        uid2 = glObjects[1];
        mappedData = nullptr;
        width = desc.width;
        height = desc.height;
        format = desc.format;
        sampleCount = desc.sampleCount;
        isCubeMap = desc.type == TEXTURE_TYPE_CUBE;
        isRenderTarget = desc.isRenderTarget;
        isRenderBuffer = is_render_buffer;
        isMapped = false;
        forceSetSamplerState = true;

        if (isRenderTarget && !isRenderBuffer)
        {
            GLint int_fmt = 0;
            GLint fmt = 0;
            GLenum type = 0;
            bool compressed = false;
            GetGLTextureFormat(format, &int_fmt, &fmt, &type, &compressed);
            DVASSERT(!compressed);

            if (isCubeMap)
            {
                DVASSERT(desc.levelCount < 16);
                unsigned cmd3Count = 2;
                GLCommand cmd3[4 + 6 * 16] =
                {
                  { GLCommand::BIND_TEXTURE, { GL_TEXTURE_CUBE_MAP, uint64(&(this->uid)) } },
                  { GLCommand::SET_ACTIVE_TEXTURE, { GL_TEXTURE0 + 0 } }
                };

                for (unsigned m = 0; m != desc.levelCount; ++m)
                {
                    GLenum face[] = { GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };
                    Size2i sz = TextureExtents(Size2i(desc.width, desc.height), m);

                    for (unsigned f = 0; f != countof(face); ++f)
                    {
                        cmd3[cmd3Count].func = GLCommand::TEX_IMAGE2D;
                        cmd3[cmd3Count].arg[0] = face[f];
                        cmd3[cmd3Count].arg[1] = m;
                        cmd3[cmd3Count].arg[2] = uint64(int_fmt);
                        cmd3[cmd3Count].arg[3] = uint64(sz.dx);
                        cmd3[cmd3Count].arg[4] = uint64(sz.dy);
                        cmd3[cmd3Count].arg[5] = 0;
                        cmd3[cmd3Count].arg[6] = uint64(fmt);
                        cmd3[cmd3Count].arg[7] = type;
                        cmd3[cmd3Count].arg[8] = 0;
                        cmd3[cmd3Count].arg[9] = 0;
                        cmd3[cmd3Count].arg[10] = 0;
                        ++cmd3Count;
                    }
                }

                if (desc.levelCount == 1)
                {
                    cmd3[cmd3Count].func = GLCommand::TEX_PARAMETER_I;
                    cmd3[cmd3Count].arg[0] = GL_TEXTURE_CUBE_MAP;
                    cmd3[cmd3Count].arg[1] = GL_TEXTURE_MAG_FILTER;
                    cmd3[cmd3Count].arg[2] = GL_NEAREST;
                    ++cmd3Count;

                    cmd3[cmd3Count].func = GLCommand::TEX_PARAMETER_I;
                    cmd3[cmd3Count].arg[0] = GL_TEXTURE_CUBE_MAP;
                    cmd3[cmd3Count].arg[1] = GL_TEXTURE_MIN_FILTER;
                    cmd3[cmd3Count].arg[2] = GL_NEAREST;
                    ++cmd3Count;
                }
                else
                {
                    cmd3[cmd3Count].func = GLCommand::TEX_PARAMETER_I;
                    cmd3[cmd3Count].arg[0] = GL_TEXTURE_CUBE_MAP;
                    cmd3[cmd3Count].arg[1] = GL_TEXTURE_MAG_FILTER;
                    cmd3[cmd3Count].arg[2] = GL_LINEAR;
                    ++cmd3Count;

                    cmd3[cmd3Count].func = GLCommand::TEX_PARAMETER_I;
                    cmd3[cmd3Count].arg[0] = GL_TEXTURE_CUBE_MAP;
                    cmd3[cmd3Count].arg[1] = GL_TEXTURE_MIN_FILTER;
                    cmd3[cmd3Count].arg[2] = GL_LINEAR;
                    ++cmd3Count;
                }

                #if !defined(__DAVAENGINE_ANDROID__)
                cmd3[cmd3Count].func = GLCommand::TEX_PARAMETER_I;
                cmd3[cmd3Count].arg[0] = GL_TEXTURE_CUBE_MAP;
                cmd3[cmd3Count].arg[1] = GL_TEXTURE_MAX_LEVEL;
                cmd3[cmd3Count].arg[2] = desc.levelCount - 1;
                ++cmd3Count;
                #endif

                cmd3[cmd3Count].func = GLCommand::RESTORE_TEXTURE0;
                ++cmd3Count;

                ExecGL(cmd3, cmd3Count, forceExecute);
            }
            else
            {
                GLCommand cmd3[] =
                {
                  { GLCommand::BIND_TEXTURE, { GL_TEXTURE_2D, uint64(&(this->uid)) } },
                  { GLCommand::TEX_IMAGE2D, { GL_TEXTURE_2D, 0, uint64(int_fmt), uint64(desc.width), uint64(desc.height), 0, uint64(fmt), type, 0, 0, 0 } },
                  { GLCommand::TEX_PARAMETER_I, { GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST } },
                  { GLCommand::TEX_PARAMETER_I, { GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST } },
                  { GLCommand::RESTORE_TEXTURE0, {} }
                };
                ExecGL(cmd3, countof(cmd3), forceExecute);
            }
        }

        success = true;
    }

    return success;
}

//------------------------------------------------------------------------------

void TextureGLES2_t::Destroy(bool forceExecute)
{
    GLCommand cmd[64] = {};
    GLint doomed_fbo[64];
    size_t cmd_cnt = 1;

    if (isRenderTarget || isRenderBuffer)
    {
        cmd[0].func = isRenderBuffer ? GLCommand::DELETE_RENDERBUFFERS : GLCommand::DELETE_TEXTURES;
        cmd[0].arg[0] = 1;
        cmd[0].arg[1] = uint64(&(uid));

        if ((uid2 != 0) && (uid2 != uid))
        {
            cmd[cmd_cnt].func = GLCommand::DELETE_RENDERBUFFERS;
            cmd[cmd_cnt].arg[0] = 1;
            cmd[cmd_cnt].arg[1] = uint64(&(uid2));
            ++cmd_cnt;
        }

        for (std::vector<FramebufferGLES2_t>::iterator f = TextureFBO.begin(); f != TextureFBO.end();)
        {
            bool do_delete = false;

            if (!isRenderBuffer)
            {
                for (const GLuint *t = f->colorUID, *t_end = f->colorUID + f->colorCount; t != t_end; ++t)
                {
                    if (*t == uid)
                    {
                        do_delete = true;
                        break;
                    }
                }
            }

            if (isRenderBuffer && f->depthStencilUID != 0 && (f->depthStencilUID == uid || f->depthStencilUID == uid2))
                do_delete = true;

            if (do_delete)
            {
                if (cmd_cnt == countof(cmd) - 1)
                {
                    ExecGL(cmd, static_cast<uint32>(cmd_cnt), forceExecute);
                    cmd_cnt = 0;
                }
                DVASSERT(cmd_cnt <= countof(cmd) - 1);
                doomed_fbo[cmd_cnt] = f->frameBuffer;
                cmd[cmd_cnt].func = GLCommand::DELETE_FRAMEBUFFERS;
                cmd[cmd_cnt].arg[0] = 1;
                cmd[cmd_cnt].arg[1] = uint64(&(doomed_fbo[cmd_cnt]));
                ++cmd_cnt;

                f = TextureFBO.erase(f);
            }
            else
            {
                ++f;
            }
        }
    }
    else
    {
        cmd[0].func = GLCommand::DELETE_TEXTURES;
        cmd[0].arg[0] = 1;
        cmd[0].arg[1] = uint64(&(uid));
    }

    if (cmd_cnt)
        ExecGL(cmd, static_cast<uint32>(cmd_cnt), forceExecute);

    DVASSERT(!isMapped);

    if (mappedData)
    {
        ::free(mappedData);

        mappedData = nullptr;
        width = 0;
        height = 0;
    }
    MarkRestored();
}

//------------------------------------------------------------------------------

static void gles2_Texture_Delete(Handle tex, bool forceExecute)
{
    TextureGLES2_t* self = TextureGLES2Pool::Get(tex);
    self->Destroy(forceExecute);
    TextureGLES2Pool::Free(tex);
}

//------------------------------------------------------------------------------

static Handle gles2_Texture_Create(const Texture::Descriptor& desc)
{
    Handle handle = TextureGLES2Pool::Alloc();
    TextureGLES2_t* tex = TextureGLES2Pool::Get(handle);

    if (tex->Create(desc) == false)
    {
        TextureGLES2Pool::Free(handle);
        handle = InvalidHandle;
    }

    return handle;
}

//------------------------------------------------------------------------------

static void* gles2_Texture_Map(Handle tex, unsigned level, TextureFace face)
{
    TextureGLES2_t* self = TextureGLES2Pool::Get(tex);
    void* data = nullptr;
    uint32 data_sz = TextureSize(self->format, self->width, self->height, level);

    DVASSERT(!self->isRenderBuffer);
    DVASSERT(!self->isMapped);

    self->mappedData = reinterpret_cast<uint8*>(::realloc(self->mappedData, data_sz));

    if (self->mappedData)
    {
        if (self->isRenderTarget)
        {
            DVASSERT(level == 0);
            DVASSERT(self->fbo != 0);
            GLint internalFormat;
            GLint format;
            GLenum type;
            bool compressed;
            GetGLTextureFormat(self->format, &internalFormat, &format, &type, &compressed);

            GLCommand cmd[] =
            {
              { GLCommand::BIND_FRAMEBUFFER, { GL_FRAMEBUFFER, uint64(self->fbo) } },
              { GLCommand::READ_PIXELS, { 0, 0, self->width, self->height, static_cast<uint64>(format), type, uint64(self->mappedData) } },
              { GLCommand::BIND_FRAMEBUFFER, { GL_FRAMEBUFFER, _GLES2_Bound_FrameBuffer } },
            };

            ExecGL(cmd, countof(cmd));

            data = self->mappedData;
            self->mappedLevel = 0;
            self->isMapped = true;
        }
        else
        {
            data = self->mappedData;
            self->mappedLevel = level;
            self->isMapped = true;
            self->mappedFace = face;
        }
    }

    if (self->format == TEXTURE_FORMAT_R4G4B4A4)
    {
        _FlipRGBA4_ABGR4(self->mappedData, self->mappedData, data_sz);
    }
    else if (self->format == TEXTURE_FORMAT_R5G5B5A1)
    {
        _ABGR1555toRGBA5551(self->mappedData, self->mappedData, data_sz);
    }

    return data;
}

//------------------------------------------------------------------------------

static void
gles2_Texture_Unmap(Handle tex)
{
    TextureGLES2_t* self = TextureGLES2Pool::Get(tex);
    Size2i sz = TextureExtents(Size2i(self->width, self->height), self->mappedLevel);
    GLint int_fmt;
    GLint fmt;
    GLenum type;
    bool compressed;

    uint32 textureDataSize = TextureSize(self->format, sz.dx, sz.dy);
    GetGLTextureFormat(self->format, &int_fmt, &fmt, &type, &compressed);

    DVASSERT(self->isMapped);
    if (self->format == TEXTURE_FORMAT_R4G4B4A4)
    {
        _FlipRGBA4_ABGR4(self->mappedData, self->mappedData, textureDataSize);
    }
    else if (self->format == TEXTURE_FORMAT_R5G5B5A1)
    {
        _RGBA5551toABGR1555(self->mappedData, self->mappedData, textureDataSize);
    }

    GLenum ttarget = (self->isCubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
    GLenum target = GL_TEXTURE_2D;

    if (self->isCubeMap)
    {
        switch (self->mappedFace)
        {
        case TEXTURE_FACE_POSITIVE_X:
            target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
            break;
        case TEXTURE_FACE_NEGATIVE_X:
            target = GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
            break;
        case TEXTURE_FACE_POSITIVE_Y:
            target = GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
            break;
        case TEXTURE_FACE_NEGATIVE_Y:
            target = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
            break;
        case TEXTURE_FACE_POSITIVE_Z:
            target = GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
            break;
        case TEXTURE_FACE_NEGATIVE_Z:
            target = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
            break;
        default:
            break;
        }
    }

    GLCommand cmd[] =
    {
      { GLCommand::SET_ACTIVE_TEXTURE, { GL_TEXTURE0 + 0 } },
      { GLCommand::BIND_TEXTURE, { ttarget, uint64(&(self->uid)) } },
      { GLCommand::TEX_IMAGE2D, { target, self->mappedLevel, uint64(int_fmt), uint64(sz.dx), uint64(sz.dy), 0, uint64(fmt), type, uint64(textureDataSize), reinterpret_cast<uint64>(self->mappedData), compressed } },
      { GLCommand::RESTORE_TEXTURE0, {} }
    };

    ExecGL(cmd, countof(cmd));

    self->isMapped = false;
    ::free(self->mappedData);
    self->mappedData = nullptr;
}

//------------------------------------------------------------------------------

void gles2_Texture_Update(Handle tex, const void* data, uint32 level, TextureFace face)
{
    TextureGLES2_t* self = TextureGLES2Pool::Get(tex);
    Size2i sz = TextureExtents(Size2i(self->width, self->height), level);
    GLint int_fmt;
    GLint fmt;
    GLenum type;
    bool compressed;
    uint32 textureDataSize = TextureSize(self->format, sz.dx, sz.dy);

    DVASSERT(!self->isRenderBuffer);
    DVASSERT(!self->isMapped);

    GetGLTextureFormat(self->format, &int_fmt, &fmt, &type, &compressed);

    if (self->format == TEXTURE_FORMAT_R4G4B4A4 || self->format == TEXTURE_FORMAT_R5G5B5A1)
    {
        gles2_Texture_Map(tex, level, face);
        memcpy(self->mappedData, data, textureDataSize);
        gles2_Texture_Unmap(tex);
    }
    else
    {
        GLenum ttarget = (self->isCubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
        GLenum target = GL_TEXTURE_2D;

        if (self->isCubeMap)
        {
            switch (face)
            {
            case TEXTURE_FACE_POSITIVE_X:
                target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
                break;
            case TEXTURE_FACE_NEGATIVE_X:
                target = GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
                break;
            case TEXTURE_FACE_POSITIVE_Y:
                target = GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
                break;
            case TEXTURE_FACE_NEGATIVE_Y:
                target = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
                break;
            case TEXTURE_FACE_POSITIVE_Z:
                target = GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
                break;
            case TEXTURE_FACE_NEGATIVE_Z:
                target = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
                break;
            default:
                DVASSERT(0, "Invalid TextureFace provided");
            }
        }

        GLCommand cmd[] =
        {
          { GLCommand::SET_ACTIVE_TEXTURE, { GL_TEXTURE0 + 0 } },
          { GLCommand::BIND_TEXTURE, { ttarget, uint64(&(self->uid)) } },
          { GLCommand::TEX_IMAGE2D, { target, uint64(level), uint64(int_fmt), uint64(sz.dx), uint64(sz.dy), 0, uint64(fmt), type, uint64(textureDataSize), reinterpret_cast<uint64>(data), compressed } },
          { GLCommand::RESTORE_TEXTURE0, {} }
        };

        ExecGL(cmd, countof(cmd));
    }
}

//------------------------------------------------------------------------------

bool gles2_Texture_NeedRestore(Handle tex)
{
    TextureGLES2_t* self = TextureGLES2Pool::Get(tex);
    return self->NeedRestore();
}

//==============================================================================

struct
SamplerStateGLES2_t
{
    SamplerState::Descriptor::Sampler fragmentSampler[MAX_FRAGMENT_TEXTURE_SAMPLER_COUNT];
    uint32 fragmentSamplerCount;
    SamplerState::Descriptor::Sampler vertexSampler[MAX_VERTEX_TEXTURE_SAMPLER_COUNT];
    uint32 vertexSamplerCount;
};

typedef ResourcePool<SamplerStateGLES2_t, RESOURCE_SAMPLER_STATE, SamplerState::Descriptor, false> SamplerStateGLES2Pool;
RHI_IMPL_POOL(SamplerStateGLES2_t, RESOURCE_SAMPLER_STATE, SamplerState::Descriptor, false);
static const SamplerStateGLES2_t* _CurSamplerState = nullptr;

//------------------------------------------------------------------------------

static Handle
gles2_SamplerState_Create(const SamplerState::Descriptor& desc)
{
    Handle handle = SamplerStateGLES2Pool::Alloc();
    SamplerStateGLES2_t* state = SamplerStateGLES2Pool::Get(handle);

    state->fragmentSamplerCount = desc.fragmentSamplerCount;
    for (unsigned i = 0; i != desc.fragmentSamplerCount; ++i)
    {
        state->fragmentSampler[i] = desc.fragmentSampler[i];
    }

    state->vertexSamplerCount = desc.vertexSamplerCount;
    for (unsigned i = 0; i != desc.vertexSamplerCount; ++i)
    {
        state->vertexSampler[i] = desc.vertexSampler[i];
    }

    return handle;
}

//------------------------------------------------------------------------------

static void
gles2_SamplerState_Delete(Handle hstate)
{
    SamplerStateGLES2_t* state = SamplerStateGLES2Pool::Get(hstate);

    if (_CurSamplerState == state)
        _CurSamplerState = nullptr;

    SamplerStateGLES2Pool::Free(hstate);
}

//==============================================================================

namespace SamplerStateGLES2
{
void Init(uint32 maxCount)
{
    SamplerStateGLES2Pool::Reserve(maxCount);
}
void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_SamplerState_Create = &gles2_SamplerState_Create;
    dispatch->impl_SamplerState_Delete = &gles2_SamplerState_Delete;
}

void SetToRHI(Handle hstate)
{
    SamplerStateGLES2_t* state = SamplerStateGLES2Pool::Get(hstate);

    _CurSamplerState = state;
}
}

//==============================================================================

static GLenum
_TextureFilterGLES2(TextureFilter filter)
{
    GLenum f = GL_LINEAR;

    switch (filter)
    {
    case TEXFILTER_NEAREST:
        f = GL_NEAREST;
        break;
    case TEXFILTER_LINEAR:
        f = GL_LINEAR;
        break;
    }

    return f;
}

//------------------------------------------------------------------------------

static GLenum
_TextureMinMipFilterGLES2(TextureFilter minfilter, TextureMipFilter mipfilter)
{
    GLenum f = GL_LINEAR_MIPMAP_LINEAR;

    switch (mipfilter)
    {
    case TEXMIPFILTER_NONE:
        f = (minfilter == TEXFILTER_NEAREST) ? GL_NEAREST : GL_LINEAR;
        break;
    case TEXMIPFILTER_NEAREST:
        f = (minfilter == TEXFILTER_NEAREST) ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_NEAREST;
        break;
    case TEXMIPFILTER_LINEAR:
        f = (minfilter == TEXFILTER_NEAREST) ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_LINEAR;
        break;
    }

    return f;
}

//------------------------------------------------------------------------------

static GLenum
_AddrModeGLES2(TextureAddrMode mode)
{
    GLenum m = GL_REPEAT;

    switch (mode)
    {
    case TEXADDR_WRAP:
        m = GL_REPEAT;
        break;
    case TEXADDR_CLAMP:
        m = GL_CLAMP_TO_EDGE;
        break;
    case TEXADDR_MIRROR:
        m = GL_MIRRORED_REPEAT;
        break;
    }

    return m;
}

//==============================================================================

namespace TextureGLES2
{
void Init(uint32 maxCount)
{
    TextureGLES2Pool::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_Texture_Create = &gles2_Texture_Create;
    dispatch->impl_Texture_Delete = &gles2_Texture_Delete;
    dispatch->impl_Texture_Map = &gles2_Texture_Map;
    dispatch->impl_Texture_Unmap = &gles2_Texture_Unmap;
    dispatch->impl_Texture_Update = &gles2_Texture_Update;
    dispatch->impl_Texture_NeedRestore = &gles2_Texture_NeedRestore;
}

void InvalidateCache()
{
    _GLES2_LastActiveTexture = -1;
}

void SetToRHI(Handle tex, unsigned unit_i, uint32 base_i)
{
    TextureGLES2_t* self = TextureGLES2Pool::Get(tex);
    bool fragment = base_i != DAVA::InvalidIndex;
    uint32 sampler_i = (base_i == DAVA::InvalidIndex) ? unit_i : base_i + unit_i;
    GLenum target = (self->isCubeMap) ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

    const SamplerState::Descriptor::Sampler* sampler = (fragment) ? _CurSamplerState->fragmentSampler + unit_i : _CurSamplerState->vertexSampler + unit_i;

    if (uint32(_GLES2_LastActiveTexture) != GL_TEXTURE0 + sampler_i)
    {
        GL_CALL(glActiveTexture(GL_TEXTURE0 + sampler_i));
        _GLES2_LastActiveTexture = GL_TEXTURE0 + sampler_i;
    }

    GL_CALL(glBindTexture(target, self->uid));

    if (sampler_i == 0)
    {
        _GLES2_LastSetTex0 = self->uid;
        _GLES2_LastSetTex0Target = target;
    }

    if ((self->forceSetSamplerState || memcmp(&(self->samplerState), sampler, sizeof(rhi::SamplerState::Descriptor::Sampler))) && !self->isRenderBuffer)
    {
        GL_CALL(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, _TextureMinMipFilterGLES2(TextureFilter(sampler->minFilter), TextureMipFilter(sampler->mipFilter))));
        GL_CALL(glTexParameteri(target, GL_TEXTURE_MAG_FILTER, _TextureFilterGLES2(TextureFilter(sampler->magFilter))));

        GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_S, _AddrModeGLES2(TextureAddrMode(sampler->addrU))));
        GL_CALL(glTexParameteri(target, GL_TEXTURE_WRAP_T, _AddrModeGLES2(TextureAddrMode(sampler->addrV))));

        if (rhi::DeviceCaps().isAnisotropicFilteringSupported())
        {
            DVASSERT(sampler->anisotropyLevel >= 1);
            DVASSERT(sampler->anisotropyLevel <= rhi::DeviceCaps().maxAnisotropy);
            GL_CALL(glTexParameteri(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, sampler->anisotropyLevel));
        }

        self->samplerState = *sampler;
        self->forceSetSamplerState = false;
    }
}

unsigned GetFrameBuffer(const Handle* color, const TextureFace* face, const unsigned* level, uint32 colorCount, Handle depthStencil)
{
    GLuint fb = 0;

    for (const FramebufferGLES2_t& f : TextureFBO)
    {
        if (f.colorCount == colorCount
            && memcmp(f.color, color, sizeof(Handle) * colorCount) == 0
            && memcmp(f.colorFace, face, sizeof(TextureFace) * colorCount) == 0
            && memcmp(f.colorLevel, level, sizeof(unsigned) * colorCount) == 0
            && (f.depthStencil == depthStencil)
            )
        {
            fb = f.frameBuffer;
            break;
        }
    }

    if (fb == 0)
    {
        GL_CALL(glGenFramebuffers(1, &fb));
        DVASSERT(fb != 0);
        GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, fb));

        for (unsigned i = 0; i != colorCount; ++i)
        {
            TextureGLES2_t* tex = TextureGLES2Pool::Get(color[i]);

            if (tex->isRenderBuffer)
            {
                DVASSERT(glIsRenderbuffer(tex->uid));
                GL_CALL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_RENDERBUFFER, tex->uid));
            }
            else
            {
                GLenum target = GL_TEXTURE_2D;
                if (tex->isCubeMap)
                {
                    switch (face[i])
                    {
                    case TextureFace::TEXTURE_FACE_POSITIVE_X:
                        target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
                        break;
                    case TextureFace::TEXTURE_FACE_NEGATIVE_X:
                        target = GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
                        break;
                    case TextureFace::TEXTURE_FACE_POSITIVE_Y:
                        target = GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
                        break;
                    case TextureFace::TEXTURE_FACE_NEGATIVE_Y:
                        target = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
                        break;
                    case TextureFace::TEXTURE_FACE_POSITIVE_Z:
                        target = GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
                        break;
                    case TextureFace::TEXTURE_FACE_NEGATIVE_Z:
                        target = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
                        break;
                    default:
                        DVASSERT(0, "Invalid TextureFace provided");
                    }
                }
                /*
                 * GFX_COMPLETE
                 * There was another function around, called "glFramebufferTextureLayer"
                 * In case of emergency or sudden break of rendering to cubemap levels or layers contact s_reznik
                 */
                GL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, target, tex->uid, level[i]));
            }
        }

        if (depthStencil != InvalidHandle && depthStencil != DefaultDepthBuffer)
        {
            TextureGLES2_t* ds = TextureGLES2Pool::Get(depthStencil);

		#if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_ANDROID__)
            GL_CALL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, ds->uid));
            if (ds->uid2)
            {
                GL_CALL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, ds->uid2));
            }
		#else
            GL_CALL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, ds->uid));
		#endif
        }

	#if !(defined __DAVAENGINE_IPHONE__ || defined __DAVAENGINE_ANDROID__)
        GLenum b[MAX_RENDER_TARGET_COUNT] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
        GL_CALL(glDrawBuffers(colorCount, b));
	#endif

        int status = 0;
        GL_CALL(status = glCheckFramebufferStatus(GL_FRAMEBUFFER));

        if (status == GL_FRAMEBUFFER_COMPLETE)
        {
            FramebufferGLES2_t f;
            f.colorCount = colorCount;
            memcpy(f.color, color, sizeof(Handle) * colorCount);
            memcpy(f.colorFace, face, sizeof(TextureFace) * colorCount);
            memcpy(f.colorLevel, level, sizeof(unsigned) * colorCount);
            f.depthStencil = depthStencil;
            f.depthStencilUID = (depthStencil != InvalidHandle && depthStencil != DefaultDepthBuffer) ? TextureGLES2Pool::Get(depthStencil)->uid : 0;
            f.frameBuffer = fb;

            for (unsigned i = 0; i != colorCount; ++i)
            {
                TextureGLES2_t* tex = TextureGLES2Pool::Get(color[i]);

                f.colorUID[i] = tex->uid;

                if (tex->fbo == 0)
                    tex->fbo = fb;
            }

            TextureFBO.push_back(f);
        }
        else
        {
            DAVA::Logger::Error("glCheckFramebufferStatus = %08X", status);
            DVASSERT(status == GL_FRAMEBUFFER_COMPLETE);
        }

        GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, _GLES2_Bound_FrameBuffer));
    }

    return fb;
}

void ResolveMultisampling(Handle fromTexture, Handle toTexture)
{
    DVASSERT(fromTexture != rhi::InvalidHandle);
    TextureGLES2_t* from = TextureGLES2Pool::Get(fromTexture);
    TextureGLES2_t* to = toTexture == rhi::InvalidHandle ? nullptr : TextureGLES2Pool::Get(toTexture);

    GLuint targetBuffer = _GLES2_Default_FrameBuffer;

    bool fromHasDepthFormat = (from->format == TextureFormat::TEXTURE_FORMAT_D16) || (from->format == TextureFormat::TEXTURE_FORMAT_D24S8);
    DVASSERT(!fromHasDepthFormat);
    //    DVASSERT(!from->fbo.empty());

    if (to != nullptr)
    {
        bool toHasDepthFormat = (to->format == TextureFormat::TEXTURE_FORMAT_D16) || (to->format == TextureFormat::TEXTURE_FORMAT_D24S8);
        DVASSERT(!toHasDepthFormat);
        //        if (to->fbo.empty())
        if (to->fbo == 0)
        {
            // force create frame-buffer object
            Handle colorTex = toTexture;
            TextureFace colorFace = TEXTURE_FACE_NONE;
            unsigned colorLevel = 0;

            GetFrameBuffer(&colorTex, &colorFace, &colorLevel, 1, InvalidHandle);
        }
        targetBuffer = to->fbo;
    }

    _GLES2_Bound_FrameBuffer = from->fbo;
    GL_CALL(glBindFramebuffer(GL_READ_FRAMEBUFFER, _GLES2_Bound_FrameBuffer));
    GL_CALL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, targetBuffer));

#if defined(__DAVAENGINE_IPHONE__)
    ios_gl_resolve_multisampling(0, 0, from->width, from->height, 0, 0, from->width, from->height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
#elif defined(__DAVAENGINE_ANDROID__)
    DVASSERT(glBlitFramebuffer != nullptr);
#endif

    GL_CALL(glBlitFramebuffer(0, 0, from->width, from->height, 0, 0, from->width, from->height, GL_COLOR_BUFFER_BIT, GL_LINEAR));
    GL_CALL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _GLES2_Bound_FrameBuffer));
}

Size2i Size(Handle tex)
{
    TextureGLES2_t* self = TextureGLES2Pool::Get(tex);

    return Size2i(self->width, self->height);
}

void ReCreateAll()
{
    TextureGLES2Pool::ReCreateAll();
}

unsigned
NeedRestoreCount()
{
    return TextureGLES2Pool::PendingRestoreCount();
}

} // namespace TextureGLES2

//==============================================================================
} // namespace rhi
