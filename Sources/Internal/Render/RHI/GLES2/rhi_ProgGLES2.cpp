#include "rhi_ProgGLES2.h"
    #include "../Common/rhi_Private.h"
    #include "../Common/dbg_StatSet.h"
    #include "../Common/rhi_Pool.h"
    #include "../Common/rhi_RingBuffer.h"

    #include "Logger/Logger.h"
using DAVA::Logger;

    #include "rhi_GLES2.h"
    #include "_gl.h"

    #include <stdio.h>
    #include <string.h>

namespace rhi
{
//==============================================================================

typedef ResourcePool<ProgGLES2::ConstBuf, RESOURCE_CONST_BUFFER, ProgGLES2::ConstBuf::Desc, false> ConstBufGLES2Pool;
RHI_IMPL_POOL_SIZE(ProgGLES2::ConstBuf, RESOURCE_CONST_BUFFER, ProgGLES2::ConstBuf::Desc, false, 12 * 1024);

static RingBuffer _GLES2_DefaultConstRingBuffer;
uint32 ProgGLES2::ConstBuf::CurFrame = 0;

//==============================================================================

static void
DumpShaderTextGLES2(const char* code, unsigned code_sz)
{
    char ss[64 * 1024];
    unsigned line_cnt = 0;

    if (code_sz < sizeof(ss))
    {
        strcpy(ss, code);

        const char* line = ss;
        for (char* s = ss; *s; ++s)
        {
            if (*s == '\r')
                *s = ' ';

            if (*s == '\n')
            {
                *s = 0;
                Logger::Info("%4u |  %s", 1 + line_cnt, line);
                line = s + 1;
                ++line_cnt;
            }
        }
    }
    else
    {
        Logger::Info(code);
    }
}

//==============================================================================

ProgGLES2::ProgGLES2(ProgType t)
    : shader(0)
    , prog(0)
    , type(t)
    , texunitCount(0)
    , texunitInited(false)
{
    for (unsigned i = 0; i != MAX_CONST_BUFFER_COUNT; ++i)
    {
        cbuf[i].location = DAVA::InvalidIndex;
        cbuf[i].count = 0;
    }

    memset(cbufLastBoundData, 0, sizeof(cbufLastBoundData));
}

//------------------------------------------------------------------------------

ProgGLES2::~ProgGLES2()
{
}

//------------------------------------------------------------------------------

bool ProgGLES2::Construct(const char* srcCode)
{
    bool success = false;
    int stype = (type == PROG_VERTEX) ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
    GLCommand cmd1 = { GLCommand::CREATE_SHADER, { uint64(stype) } };

    ExecGL(&cmd1, 1);

    if (cmd1.retval)
    {
        unsigned s = cmd1.retval;
        int status = 0;
        char info[1024] = "";
        GLCommand cmd2[] =
        {
          { GLCommand::SHADER_SOURCE, { s, 1, uint64(&srcCode), 0 } },
          { GLCommand::COMPILE_SHADER, { s } },
          { GLCommand::GET_SHADER_IV, { s, GL_COMPILE_STATUS, uint64(&status) } },
          { GLCommand::GET_SHADER_INFO_LOG, { s, countof(info), 0, uint64(info) } }
        };

        ExecGL(cmd2, countof(cmd2));

        if (status)
        {
            shader = s;
            success = true;
        }
        else
        {
            Logger::Error("%sprog-compile failed:", (type == PROG_VERTEX) ? "v" : "f");
            Logger::Error(info);
            DumpShaderTextGLES2(srcCode, static_cast<unsigned>(strlen(srcCode)));
        }

        memset(cbufLastBoundData, 0, sizeof(cbufLastBoundData));
    }

    return success;
}

//------------------------------------------------------------------------------

void ProgGLES2::Destroy()
{
}

//------------------------------------------------------------------------------

void ProgGLES2::GetProgParams(unsigned progUid)
{
#if RHI_GL__USE_UNIFORMBUFFER_OBJECT
    for (unsigned i = 0; i != MAX_CONST_BUFFER_COUNT; ++i)
    {
        char name[32];
        sprintf(name, "%s_Buffer%u_Block", (type == PROG_VERTEX) ? "VP" : "FP", i);
        GLuint loc;
        GL_CALL(loc = glGetUniformBlockIndex(progUid, name));

        if (loc != GL_INVALID_INDEX)
        {
            GLint sz;

            GL_CALL(glGetActiveUniformBlockiv(progUid, loc, GL_UNIFORM_BLOCK_DATA_SIZE, &sz));
            GL_CALL(glUniformBlockBinding(progUid, loc, loc));

            cbuf[i].location = loc;
            cbuf[i].count = sz / (4 * sizeof(float));
        }
        else
        {
            cbuf[i].location = DAVA::InvalidIndex;
            cbuf[i].count = 0;
        }
    }
#else

    for (unsigned i = 0; i != MAX_CONST_BUFFER_COUNT; ++i)
    {
        cbuf[i].location = DAVA::InvalidIndex;
        cbuf[i].count = 0;
    }

    GLint cnt = 0;
    GLCommand cmd1 = { GLCommand::GET_PROGRAM_IV, { progUid, GL_ACTIVE_UNIFORMS, uint64(&cnt) } };

    ExecGL(&cmd1, 1);

    for (unsigned u = 0; u != cnt; ++u)
    {
        char name[64];
        GLsizei length;
        GLint size;
        GLenum utype;
        GLCommand cmd2 = { GLCommand::GET_ACTIVE_UNIFORM, { progUid, u, uint64(sizeof(name) - 1), uint64(&length), uint64(&size), uint64(&utype), uint64(name) } };

        ExecGL(&cmd2, 1);

        for (unsigned i = 0; i != MAX_CONST_BUFFER_COUNT; ++i)
        {
            char n[16], n2[16];
            sprintf(n, "%s_Buffer%u[0]", (type == PROG_VERTEX) ? "VP" : "FP", i);
            sprintf(n2, "%s_Buffer%u", (type == PROG_VERTEX) ? "VP" : "FP", i);

            if (!strcmp(name, n) || !strcmp(name, n2))
            {
                int loc;
                GLCommand cmd3 = { GLCommand::GET_UNIFORM_LOCATION, { progUid, uint64(name) } };

                ExecGL(&cmd3, 1);
                loc = cmd3.retval;

                if (loc != -1)
                {
                    cbuf[i].location = loc;
                    cbuf[i].count = size;
                    break;
                }
            }
        }
    }
#endif // RHI_GL__USE_UNIFORMBUFFER_OBJECT

    // get texture location
    {
        char tname[countof(texunitLoc)][32];
        GLCommand cmd[countof(texunitLoc)];

        for (unsigned i = 0; i != countof(texunitLoc); ++i)
        {
            if (type == PROG_FRAGMENT)
                Snprintf(tname[i], countof(tname[i]), "FragmentTexture%u", i);
            else if (type == PROG_VERTEX)
                Snprintf(tname[i], countof(tname[i]), "VertexTexture%u", i);

            cmd[i].func = GLCommand::GET_UNIFORM_LOCATION;
            cmd[i].arg[0] = progUid;
            cmd[i].arg[1] = uint64(tname[i]);
        }

        ExecGL(cmd, countof(cmd));
        texunitCount = 0;

        for (unsigned i = 0; i != countof(texunitLoc); ++i)
        {
            int loc = cmd[i].retval;

            texunitLoc[i] = (loc != -1) ? loc : DAVA::InvalidIndex;

            if (loc != -1)
                ++texunitCount;
        }
    }

    prog = progUid;
    texunitInited = true;
}

//------------------------------------------------------------------------------

unsigned
ProgGLES2::SamplerCount() const
{
    return texunitCount;
}

//------------------------------------------------------------------------------

unsigned
ProgGLES2::ConstBufferCount() const
{
    return countof(cbuf);
}

//------------------------------------------------------------------------------

Handle
ProgGLES2::InstanceConstBuffer(unsigned bufIndex) const
{
    Handle handle = InvalidHandle;

    DVASSERT(bufIndex < countof(cbuf));
    DVASSERT(prog != 0);
    //    DVASSERT(cbuf[bufIndex].location != DAVA::InvalidIndex);

    if (bufIndex < countof(cbuf) && cbuf[bufIndex].location != DAVA::InvalidIndex)
    {
        handle = ConstBufGLES2Pool::Alloc();

        ConstBuf* cb = ConstBufGLES2Pool::Get(handle);

        void** data = const_cast<void**>(&cbufLastBoundData[bufIndex]);
        if (!cb->Construct(prog, data, cbuf[bufIndex].location, cbuf[bufIndex].count))
        {
            ConstBufGLES2Pool::Free(handle);
            handle = InvalidHandle;
        }
    }

    return handle;
}

//------------------------------------------------------------------------------

void ProgGLES2::SetupTextureUnits(uint32 baseUnit, GLCommand* commands, uint32& commandsCount) const
{
    for (unsigned i = 0; i != countof(texunitLoc); ++i)
    {
        if (texunitLoc[i] != -1)
        {
            commands[commandsCount] = { GLCommand::SET_UNIFORM_1I, { texunitLoc[i], baseUnit + i } };
            ++commandsCount;
        }
    }
}

//------------------------------------------------------------------------------

bool ProgGLES2::ConstBuf::Construct(uint32 prog, void** lastBoundData, unsigned loc, unsigned cnt)
{
    bool success = true;

    glProg = prog;
    location = static_cast<uint16>(loc);
    count = static_cast<uint16>(cnt);
    data = reinterpret_cast<float*>(::malloc(cnt * 4 * sizeof(float)));
    inst = nullptr;
    lastInst = lastBoundData;
    *lastInst = nullptr;
    frame = 0;

    #if RHI_GL__USE_STATIC_CONST_BUFFER_OPTIMIZATION
    isStatic = true;
    isUsedInDrawCall = false;
    lastmodifiedFrame = 0;
    altData.reserve(4);
    #if RHI_GL__DEBUG_CONST_BUFFERS
    isTrueStatic = true;
    instCount = 0;
    #endif
    #endif

    return success;
}

//------------------------------------------------------------------------------

void ProgGLES2::ConstBuf::Destroy()
{
    if (data)
    {
        ::free(data);
        #if RHI_GL__USE_STATIC_CONST_BUFFER_OPTIMIZATION
        for (std::vector<float *>::iterator d = altData.begin(), d_end = altData.end(); d != d_end; ++d)
            ::free(*d);
        altData.clear();
        #endif

        data = nullptr;
        inst = nullptr;
        lastInst = nullptr;
        location = -1;
        count = 0;
    }
}

//------------------------------------------------------------------------------

unsigned
ProgGLES2::ConstBuf::ConstCount() const
{
    return count;
}

void ProgGLES2::ConstBuf::ReallocIfneeded()
{   
#if RHI_GL__USE_STATIC_CONST_BUFFER_OPTIMIZATION
    lastmodifiedFrame = CurFrame;
    #if RHI_GL__DEBUG_CONST_BUFFERS
    if (isUsedInDrawCall)
        isTrueStatic = false;
    #endif
#endif

#if RHI_GL__USE_STATIC_CONST_BUFFER_OPTIMIZATION
    if (isStatic && isUsedInDrawCall)
    {
        float* old_data = data;

        altData.push_back(data);
        altDataAllocationFrame.push_back(CurFrame);
        data = (float*)(::malloc(count * 4 * sizeof(float)));
        memcpy(data, old_data, count * 4 * sizeof(float));
        isStatic = false;
    }
#endif

    inst = nullptr;
}

//------------------------------------------------------------------------------

bool ProgGLES2::ConstBuf::SetConst(unsigned const_i, unsigned const_count, const float* cdata)
{
    bool success = false;

    float* d = data + const_i * 4;
    float* d_end = (data + const_i * 4 + const_count * 4);
    float* end = data + count * 4;

    // this is workaround against too clever GLSL-compilers (Tegra),
    // when actual cbuf-array size is smaller that declared due to unused last elements
    if (d_end >= end)
        d_end = end;

    if (d < d_end)
    {
        memcpy(d, cdata, (d_end - d) * sizeof(float));
        ReallocIfneeded();
        success = true;
    }

    return success;
}

//------------------------------------------------------------------------------

bool ProgGLES2::ConstBuf::SetConst(unsigned const_i, unsigned const_sub_i, const float* cdata, unsigned data_count)
{
    bool success = false;

    if (const_i < count && const_sub_i < 4)
    {
        memcpy(data + const_i * 4 + const_sub_i, cdata, data_count * sizeof(float));
        ReallocIfneeded();
        success = true;
    }

    return success;
}

//------------------------------------------------------------------------------

const void*
ProgGLES2::ConstBuf::Instance() const
{
    if (frame != CurFrame)
    {
        inst = nullptr;
        *lastInst = nullptr;
    }

    if (!inst)
    {
#if RHI_GL__USE_STATIC_CONST_BUFFER_OPTIMIZATION
        // try to make 'static'
        if (ProgGLES2::ConstBuf::CurFrame - lastmodifiedFrame > 3 && altData.size() < altData.capacity() - 1)
        {
            isStatic = true;
        }

        if (isStatic)
        {
            inst = data;

            // try to release some alt-data
            for (unsigned i = 0; i != altData.size(); ++i)
            {
                if (CurFrame - altDataAllocationFrame[i] > 5)
                {
                    ::free(altData[i]);
                    altData.erase(altData.begin() + i);
                    altDataAllocationFrame.erase(altDataAllocationFrame.begin() + i);
                    break;
                }
            }
        }
        else
        {
#endif
            inst = _GLES2_DefaultConstRingBuffer.Alloc(count * 4);
            memcpy(inst, data, 4 * count * sizeof(float));
#if RHI_GL__USE_STATIC_CONST_BUFFER_OPTIMIZATION
        }
#endif

        frame = CurFrame;
        #if RHI_GL__DEBUG_CONST_BUFFERS
        ++instCount;
        #endif
    }

    #if RHI_GL__USE_STATIC_CONST_BUFFER_OPTIMIZATION
    isUsedInDrawCall = true;
    #endif

    return inst;
}

//------------------------------------------------------------------------------

void ProgGLES2::ConstBuf::SetToRHI(uint32 progUid, const void* instData) const
{
    DVASSERT(progUid == glProg);
    if (instData != *lastInst)
    {
        GLfloat* data = reinterpret_cast<GLfloat*>(const_cast<void*>(instData));
        GL_CALL(glUniform4fv(location, count, data));
        *lastInst = const_cast<void*>(instData);
    }

    StatSet::IncStat(stat_SET_CB, 1);
}

//------------------------------------------------------------------------------

void ProgGLES2::ConstBuf::AdvanceFrame()
{
    ++CurFrame;
}

//------------------------------------------------------------------------------

unsigned
ProgGLES2::ShaderUid() const
{
    return shader;
}

//------------------------------------------------------------------------------

void ProgGLES2::InvalidateAllConstBufferInstances()
{
    ConstBuf::AdvanceFrame();
    _GLES2_DefaultConstRingBuffer.Reset();

#if RHI_GL__DEBUG_CONST_BUFFERS
    unsigned staticCnt = 0;
    unsigned totalCnt = 0;
    unsigned trueStaticCnt = 0;

    unsigned dynCnt = 0;
    unsigned dynInstCnt = 0;
    unsigned dynInstSz = 0;
    unsigned staticInstCnt = 0;
    unsigned staticInstSz = 0;

    unsigned staticSz = 0;
    unsigned totalSz = 0;

    unsigned altDataCnt = 0;
    unsigned altDataSz = 0;

    for (ConstBufGLES2Pool::Iterator b = ConstBufGLES2Pool::Begin(), b_end = ConstBufGLES2Pool::End(); b != b_end; ++b)
    {
        ++totalCnt;
        if (ProgGLES2::ConstBuf::CurFrame - b->lastmodifiedFrame > 3)
        {
            //            ++staticCnt;
            staticSz += b->count * 4 * sizeof(float);
        }
        if (b->isTrueStatic)
            ++trueStaticCnt;
        
#if RHI_GL__USE_STATIC_CONST_BUFFER_OPTIMIZATION
        if (b->altData.size())
        {
            altDataCnt += b->altData.size();
            altDataSz += b->altData.size() * b->count * 4 * sizeof(float);
        }
#endif

#if RHI_GL__USE_STATIC_CONST_BUFFER_OPTIMIZATION
        if (b->isStatic)
        {
            ++staticCnt;
            staticInstCnt += b->instCount;
            if (b->instCount)
                staticInstSz += b->count * 4 * sizeof(float);
        }
        else
#endif
        {
            ++dynCnt;
            dynInstCnt += b->instCount;
            dynInstSz += b->instCount * b->count * 4 * sizeof(float);
        }

        totalSz += b->count * 4 * sizeof(float);

        b->instCount = 0;
    }

    Logger::Info("'static' const-buffers: %u ( %.2fKb in %u instances )", staticCnt, float(staticInstSz) / 1024.0f, staticInstCnt);
    Logger::Info("'dynamic' const-buffers: %u ( %.2fKb in %u instances )", dynCnt, float(dynInstSz) / 1024.0f, dynInstCnt);
    //    Logger::Info("'static' const-buffers: %u / %u  (%.2fKb / %.2fKb)",staticCnt,totalCnt,float(staticSz)/1024.0f,float(totalSz)/1024.0f);
    //    Logger::Info("'true-static' const-buffers: %u / %u",trueStaticCnt,totalCnt);
    Logger::Info("static-buf-overhead : %.2fKb in %u blocks", float(altDataSz) / 1024.0f, altDataCnt);
#endif
}

//------------------------------------------------------------------------------

static bool
gles2_ConstBuffer_SetConst(Handle cb, unsigned const_i, unsigned const_count, const float* data)
{
    ProgGLES2::ConstBuf* self = ConstBufGLES2Pool::Get(cb);

    //Logger::Info( "  upd-cb %u", unsigned(RHI_HANDLE_INDEX(cb)) );
    return self->SetConst(const_i, const_count, data);
}

//------------------------------------------------------------------------------

static bool
gles2_ConstBuffer_SetConst1(Handle cb, unsigned const_i, unsigned const_sub_i, const float* data, unsigned dataCount)
{
    ProgGLES2::ConstBuf* self = ConstBufGLES2Pool::Get(cb);

    //Logger::Info( "  upd-cb %u", unsigned(RHI_HANDLE_INDEX(cb)) );
    return self->SetConst(const_i, const_sub_i, data, dataCount);
}

//------------------------------------------------------------------------------

static void
gles2_ConstBuffer_Delete(Handle cb)
{
    ProgGLES2::ConstBuf* self = ConstBufGLES2Pool::Get(cb);

    self->Destroy();
    ConstBufGLES2Pool::Free(cb);
}

//------------------------------------------------------------------------------

namespace ConstBufferGLES2
{
void Init(uint32 maxCount)
{
    ConstBufGLES2Pool::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_ConstBuffer_SetConst = &gles2_ConstBuffer_SetConst;
    dispatch->impl_ConstBuffer_SetConst1fv = &gles2_ConstBuffer_SetConst1;
    dispatch->impl_ConstBuffer_Delete = &gles2_ConstBuffer_Delete;
}

void InitializeRingBuffer(uint32 size)
{
    _GLES2_DefaultConstRingBuffer.Initialize(size);
}

void SetToRHI(const Handle cb, uint32 progUid, const void* instData)
{
    const ProgGLES2::ConstBuf* self = ConstBufGLES2Pool::Get(cb);

    //Logger::Info( "  set-cb %u  inst= %p", unsigned(RHI_HANDLE_INDEX(cb)), instData );
    self->SetToRHI(progUid, instData);
}

const void* Instance(Handle cb)
{
    const ProgGLES2::ConstBuf* self = ConstBufGLES2Pool::Get(cb);

    return self->Instance();
}
}

//==============================================================================
} // namespace rhi
