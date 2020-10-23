    #include "../Common/rhi_Private.h"
#include "../Common/rhi_Pool.h"
#include "../Common/rhi_Utils.h"
#include "../Common/rhi_RingBuffer.h"
#include "../rhi_ShaderCache.h"
#include "rhi_DX9.h"

#include "Debug/DVAssert.h"
#include "Logger/Logger.h"
using DAVA::Logger;
using DAVA::uint32;
using DAVA::uint16;
using DAVA::uint8;

#include "_dx9.h"
#include <D3DX9Shader.h>

#include <vector>

namespace rhi
{
struct
VDeclDX9
{
    VertexLayout layout;
    IDirect3DVertexDeclaration9* vdecl9;

    static IDirect3DVertexDeclaration9* Get(const VertexLayout& layout, bool forceExecute = false);

private:
    static std::vector<VDeclDX9> _VDecl;
};
std::vector<VDeclDX9> VDeclDX9::_VDecl;

static RingBuffer _DX9_DefConstRingBuf;

static void DumpShaderTextDX9(const char* code, unsigned code_sz)
{
    char src[64 * 1024];
    char* src_line[1024];
    unsigned line_cnt = 0;

    if (code_sz < sizeof(src))
    {
        memcpy(src, code, code_sz);
        src[code_sz] = '\0';
        memset(src_line, 0, sizeof(src_line));

        src_line[line_cnt++] = src;
        for (char* s = src; *s;)
        {
            if (*s == '\n')
            {
                *s = 0;
                ++s;

                while (*s && (/**s == '\n'  ||  */ *s == '\r'))
                {
                    *s = 0;
                    ++s;
                }

                if (!(*s))
                    break;

                src_line[line_cnt] = s;
                ++line_cnt;
            }
            else if (*s == '\r')
            {
                *s = ' ';
            }
            else
            {
                ++s;
            }
        }

        for (unsigned i = 0; i != line_cnt; ++i)
        {
            Logger::Info("%4u |  %s", 1 + i, src_line[i]);
        }
    }
    else
    {
        Logger::Info(code);
    }
}

//------------------------------------------------------------------------------

IDirect3DVertexDeclaration9* VDeclDX9::Get(const VertexLayout& layout, bool forceExecute)
{
    IDirect3DVertexDeclaration9* vdecl = nullptr;

    for (std::vector<VDeclDX9>::const_iterator v = _VDecl.begin(), v_end = _VDecl.end(); v != v_end; ++v)
    {
        if (v->layout == layout)
        {
            vdecl = v->vdecl9;
            break;
        }
    }

    if (!vdecl)
    {
        IDirect3DVertexDeclaration9* vd9 = nullptr;
        D3DVERTEXELEMENT9 elem[32];
        uint32 elemCount = 0;

        DVASSERT(layout.ElementCount() < countof(elem));
        for (unsigned i = 0; i != layout.ElementCount(); ++i)
        {
            if (layout.ElementSemantics(i) == VS_PAD)
                continue;

            elem[elemCount].Stream = layout.ElementStreamIndex(i);
            elem[elemCount].Offset = (WORD)(layout.ElementOffset(i));
            elem[elemCount].Method = D3DDECLMETHOD_DEFAULT;
            elem[elemCount].UsageIndex = layout.ElementSemanticsIndex(i);

            switch (layout.ElementSemantics(i))
            {
            case VS_POSITION:
                elem[elemCount].Usage = D3DDECLUSAGE_POSITION;
                break;
            case VS_NORMAL:
                elem[elemCount].Usage = D3DDECLUSAGE_NORMAL;
                break;
            case VS_COLOR:
                elem[elemCount].Usage = D3DDECLUSAGE_COLOR;
                break;
            case VS_TEXCOORD:
                elem[elemCount].Usage = D3DDECLUSAGE_TEXCOORD;
                break;
            case VS_TANGENT:
                elem[elemCount].Usage = D3DDECLUSAGE_TANGENT;
                break;
            case VS_BINORMAL:
                elem[elemCount].Usage = D3DDECLUSAGE_BINORMAL;
                break;
            case VS_BLENDWEIGHT:
                elem[elemCount].Usage = D3DDECLUSAGE_BLENDWEIGHT;
                break;
            case VS_BLENDINDEX:
                elem[elemCount].Usage = D3DDECLUSAGE_BLENDINDICES;
                break;
            default:
                break;
            }

            switch (layout.ElementDataType(i))
            {
            case VDT_FLOAT:
            {
                switch (layout.ElementDataCount(i))
                {
                case 4:
                    elem[elemCount].Type = D3DDECLTYPE_FLOAT4;
                    break;
                case 3:
                    elem[elemCount].Type = D3DDECLTYPE_FLOAT3;
                    break;
                case 2:
                    elem[elemCount].Type = D3DDECLTYPE_FLOAT2;
                    break;
                case 1:
                    elem[elemCount].Type = D3DDECLTYPE_FLOAT1;
                    break;
                }
            }
            break;
            default:
                break;
            }

            if (layout.ElementSemantics(i) == VS_COLOR)
            {
                elem[elemCount].Type = D3DDECLTYPE_D3DCOLOR;
            }

            ++elemCount;
        }
        elem[elemCount].Stream = 0xFF;
        elem[elemCount].Offset = 0;
        elem[elemCount].Type = D3DDECLTYPE_UNUSED;
        elem[elemCount].Method = 0;
        elem[elemCount].Usage = 0;
        elem[elemCount].UsageIndex = 0;

        DX9Command cmd = { DX9Command::CREATE_VERTEX_DECLARATION, { uint64_t(elem), uint64_t(&vd9) } };

        ExecDX9(&cmd, 1, forceExecute);

        if (SUCCEEDED(cmd.retval))
        {
            VDeclDX9 vd;

            vd.vdecl9 = vd9;
            vd.layout = layout;
            _VDecl.push_back(vd);

            vdecl = vd9;
        }
        else
        {
            Logger::Error("FAILED to create vertex-decl:\n%s\n", D3D9ErrorText(cmd.retval));
            layout.Dump();
        }
    }

    return vdecl;
}

//==============================================================================

class PipelineStateDX9_t
{
public:
    PipelineStateDX9_t()
    {
    }

    class
    ConstBuf
    {
    public:
        struct Desc
        {
        };

        ~ConstBuf();

        void Construct(ProgType type, unsigned reg_i, unsigned reg_count, unsigned elem_count);
        void Destroy();

        unsigned ConstElementCount() const;
        const void* InstData();
        void InvalidateInst();

        bool SetConst(unsigned const_i, unsigned count, const float* data);
        bool SetConst(unsigned const_i, unsigned const_sub_i, const float* data, unsigned dataCount);
        void SetToRHI(const void* inst_data) const;

    private:
        ProgType progType = PROG_VERTEX;
        float* value = nullptr;
        float* inst = nullptr;
        unsigned elementCount = 0;
        unsigned registerBase = 0;
        unsigned registerCount = 0;
    };

    struct
    VertexProgDX9
    {
    public:
        VertexProgDX9()
            : stride(0)
            , codeSiize(0)
            , code(nullptr)
            , vs9(nullptr)
            , vdecl9(nullptr)
        {
        }

        bool Construct(const void* code, unsigned code_sz, const VertexLayout& vdecl);
        Handle CreateConstBuffer(unsigned buf_i);
        void SetToRHI(uint32 layoutUID, bool forceExecute = false);
        void SetupVertexStreams(uint32 layoutUID, unsigned instCount);

        struct
        vdecl_t
        {
            uint32 layoutUID;
            VertexLayout layout;
            IDirect3DVertexDeclaration9* vdecl;
        };

        VertexLayout vertexLayout;
        unsigned stride;
        unsigned codeSiize;
        void* code;
        IDirect3DVertexShader9* vs9;
        IDirect3DVertexDeclaration9* vdecl9; // ref-only
        std::vector<vdecl_t> altVdecl9;
        unsigned cbufElemCount[MAX_CONST_BUFFER_COUNT];
        unsigned cbufRegBase[MAX_CONST_BUFFER_COUNT];
        unsigned cbufRegCount[MAX_CONST_BUFFER_COUNT];
        DAVA::FastName uid;
    };

    struct
    FragmentProgDX9
    {
    public:
        FragmentProgDX9()
            : codeSize(0)
            , code(nullptr)
            , ps9(nullptr)
        {
        }

        bool Construct(const void* code, unsigned code_sz);
        Handle CreateConstBuffer(unsigned buf_i);
        void SetToRHI();

        unsigned codeSize;
        void* code;
        IDirect3DPixelShader9* ps9;
        unsigned cbufElemCount[MAX_CONST_BUFFER_COUNT];
        unsigned cbufRegBase[MAX_CONST_BUFFER_COUNT];
        unsigned cbufRegCount[MAX_CONST_BUFFER_COUNT];
        DAVA::FastName uid;
    };

    VertexProgDX9 vprog;
    FragmentProgDX9 fprog;

    DWORD blendSrc;
    DWORD blendDst;
    bool blendEnabled;
    DWORD colorMask;
};

typedef ResourcePool<PipelineStateDX9_t, RESOURCE_PIPELINE_STATE, PipelineState::Descriptor, true> PipelineStateDX9Pool;
typedef ResourcePool<PipelineStateDX9_t::ConstBuf, RESOURCE_CONST_BUFFER, PipelineStateDX9_t::ConstBuf::Desc, false> ConstBufDX9Pool;

RHI_IMPL_POOL(PipelineStateDX9_t, RESOURCE_PIPELINE_STATE, PipelineState::Descriptor, true);
RHI_IMPL_POOL_SIZE(PipelineStateDX9_t::ConstBuf, RESOURCE_CONST_BUFFER, PipelineStateDX9_t::ConstBuf::Desc, false, 12 * 1024);

//------------------------------------------------------------------------------

PipelineStateDX9_t::ConstBuf::~ConstBuf()
{
    if (value)
    {
        ::free(value);
        value = nullptr;
    }
}

//------------------------------------------------------------------------------

void PipelineStateDX9_t::ConstBuf::Construct(ProgType ptype, unsigned reg_i, unsigned reg_count, unsigned elem_count)
{
    DVASSERT(!value);
    DVASSERT(elem_count);
    DVASSERT(reg_i != DAVA::InvalidIndex);
    DVASSERT(reg_count);

    elementCount = elem_count;
    registerBase = reg_i;
    registerCount = reg_count;

    progType = ptype;
    value = (float*)(malloc(elementCount * 4 * sizeof(float)));
    inst = nullptr;
}

//------------------------------------------------------------------------------

void PipelineStateDX9_t::ConstBuf::Destroy()
{
    if (value)
    {
        ::free(value);

        value = nullptr;
        inst = nullptr;
        elementCount = 0;
        registerBase = 0;
        registerCount = 0;
    }
}

//------------------------------------------------------------------------------

unsigned
PipelineStateDX9_t::ConstBuf::ConstElementCount() const
{
    return elementCount;
}

//------------------------------------------------------------------------------

const void*
PipelineStateDX9_t::ConstBuf::InstData()
{
    if (!inst)
    {
        inst = _DX9_DefConstRingBuf.Alloc(4 * elementCount);
        memcpy(inst, value, elementCount * 4 * sizeof(float));
    }

    return inst;
}

//------------------------------------------------------------------------------

void PipelineStateDX9_t::ConstBuf::InvalidateInst()
{
    inst = nullptr;
}

//------------------------------------------------------------------------------

bool PipelineStateDX9_t::ConstBuf::SetConst(unsigned const_i, unsigned const_count, const float* data)
{
    bool success = false;

    if (const_i + const_count <= elementCount)
    {
        memcpy(value + const_i * 4, data, const_count * 4 * sizeof(float));
        inst = nullptr;
        success = true;
    }

    return success;
}

//------------------------------------------------------------------------------

bool PipelineStateDX9_t::ConstBuf::SetConst(unsigned const_i, unsigned const_sub_i, const float* data, unsigned dataCount)
{
    bool success = false;

    if (const_i <= elementCount && const_sub_i < 4)
    {
        memcpy(value + const_i * 4 + const_sub_i, data, dataCount * sizeof(float));
        inst = nullptr;
        success = true;
    }

    return success;
}

//------------------------------------------------------------------------------

void PipelineStateDX9_t::ConstBuf::SetToRHI(const void* inst_data) const
{
    HRESULT hr;

    if (progType == PROG_VERTEX)
        hr = _D3D9_Device->SetVertexShaderConstantF(registerBase, (const float*)inst_data, registerCount);
    else
        hr = _D3D9_Device->SetPixelShaderConstantF(registerBase, (const float*)inst_data, registerCount);

    DVASSERT(SUCCEEDED(hr));
}

//------------------------------------------------------------------------------

bool PipelineStateDX9_t::VertexProgDX9::Construct(const void* bin, unsigned bin_sz, const VertexLayout& vdecl)
{
    bool success = false;
    LPD3DXBUFFER shader = NULL;
    LPD3DXBUFFER err = NULL;
    LPD3DXCONSTANTTABLE const_tab = NULL;
    HRESULT hr = D3DXCompileShader(
    (const char*)bin, bin_sz,
    NULL, // no defines
    NULL, // no include-interfaces
    "vp_main",
    "vs_3_0",
    0,
    &shader,
    &err,
    &const_tab);

    if (SUCCEEDED(hr))
    {
        //DumpShaderText((const char*)bin,bin_sz);
        void* code = shader->GetBufferPointer();
        DX9Command cmd = { DX9Command::CREATE_VERTEX_SHADER, { uint64_t((const DWORD*)code), uint64_t(&vs9) } };

        ExecDX9(&cmd, 1, false);

        if (SUCCEEDED(cmd.retval))
        {
            for (unsigned i = 0; i != MAX_CONST_BUFFER_COUNT; ++i)
            {
                char name[16];
                sprintf(name, "VP_Buffer%u", i);
                D3DXHANDLE c = const_tab->GetConstantByName(NULL, name);

                cbufElemCount[i] = 0;
                cbufRegBase[i] = DAVA::InvalidIndex;
                cbufRegCount[i] = 0;

                if (c)
                {
                    D3DXCONSTANT_DESC desc;
                    UINT cnt = 1;

                    hr = const_tab->GetConstantDesc(c, &desc, &cnt);

                    if (SUCCEEDED(hr))
                    {
                        cbufElemCount[i] = desc.Elements;
                        cbufRegBase[i] = desc.RegisterIndex;
                        cbufRegCount[i] = desc.RegisterCount;
                    }
                }
                else
                {
                    if (strstr((const char*)bin, name))
                    {
                        Logger::Warning("shader has \"%s\", but no variables actually use it in code:\n", name);
                        DumpShaderTextDX9((const char*)bin, bin_sz);
                    }
                }
            }

            // do some additional sanity checks
            for (unsigned i = 0; i != MAX_CONST_BUFFER_COUNT; ++i)
            {
                if (cbufRegBase[i] == DAVA::InvalidIndex)
                {
                    if (i == 0 && cbufRegBase[i + 1] != DAVA::InvalidIndex)
                    {
                        Logger::Warning("WARNING: vertex-const-buf [%u] is unused (all uniform/variables are unused)", i);
                    }
                }
            }

            vdecl9 = VDeclDX9::Get(vdecl);
            vertexLayout = vdecl;

            DVASSERT(vdecl9);
            success = true;
        }
        else
        {
            Logger::Error("FAILED to create vertex-shader:\n%s\n", D3D9ErrorText(hr));
        }
    }
    else
    {
        Logger::Error("FAILED to compile vertex-shader:");
        if (err)
        {
            Logger::Info((const char*)(err->GetBufferPointer()));
        }
        Logger::Error("shader-uid : %s", uid.c_str());
        Logger::Error("vertex-shader text:\n");
        DumpShaderTextDX9((const char*)bin, bin_sz);
    }

    return success;
}

//------------------------------------------------------------------------------

Handle
PipelineStateDX9_t::VertexProgDX9::CreateConstBuffer(unsigned buf_i)
{
    Handle handle = InvalidHandle;

    DVASSERT(buf_i < MAX_CONST_BUFFER_COUNT);

    if (buf_i < MAX_CONST_BUFFER_COUNT)
    {
        handle = ConstBufDX9Pool::Alloc();

        ConstBuf* cb = ConstBufDX9Pool::Get(handle);

        cb->Construct(PROG_VERTEX, cbufRegBase[buf_i], cbufRegCount[buf_i], cbufElemCount[buf_i]);
    }

    return handle;
}

//------------------------------------------------------------------------------

void PipelineStateDX9_t::VertexProgDX9::SetToRHI(uint32 layoutUID, bool forceExecute)
{
    HRESULT hr = _D3D9_Device->SetVertexShader(vs9);

    if (SUCCEEDED(hr))
    {
        IDirect3DVertexDeclaration9* vd = vdecl9;
        const VertexLayout* vl = &vertexLayout;

        if (layoutUID != VertexLayout::InvalidUID)
        {
            bool do_add = true;

            for (std::vector<vdecl_t>::iterator i = altVdecl9.begin(), i_end = altVdecl9.end(); i != i_end; ++i)
            {
                if (i->layoutUID == layoutUID)
                {
                    vd = i->vdecl;
                    vl = &(i->layout);
                    do_add = false;
                    break;
                }
            }

            if (do_add)
            {
                const VertexLayout* vbLayout = VertexLayout::Get(layoutUID);
                vdecl_t info;
                VertexLayout layout;

                if (VertexLayout::MakeCompatible(*vbLayout, this->vertexLayout, &layout))
                {
                    info.vdecl = VDeclDX9::Get(layout, forceExecute);
                    info.layoutUID = layoutUID;
                    info.layout = layout;

                    altVdecl9.push_back(info);
                    vd = info.vdecl;
                    vl = &(altVdecl9.back().layout);
                }
                else
                {
                    Logger::Warning("can't create compatible vertex-layout");
                    Logger::Info("vprog-layout:");
                    this->vertexLayout.Dump();
                    Logger::Info("custom-layout:");
                    vbLayout->Dump();
                }
            }
        }

        hr = _D3D9_Device->SetVertexDeclaration(vd);

        for (uint32 s = 0; s != vl->StreamCount(); ++s)
        {
            switch (vl->StreamFrequency(s))
            {
            case VDF_PER_VERTEX:
                _D3D9_Device->SetStreamSourceFreq(s, 1);
                break;
            case VDF_PER_INSTANCE:
                _D3D9_Device->SetStreamSourceFreq(s, 1);
                break;
            }
        }

        if (FAILED(hr))
            Logger::Error("SetVertexDeclaration failed:\n%s\n", D3D9ErrorText(hr));
    }
    else
    {
        Logger::Error("SetVertexShader failed:\n%s\n", D3D9ErrorText(hr));
    }
}

//------------------------------------------------------------------------------

void
PipelineStateDX9_t::VertexProgDX9::SetupVertexStreams(uint32 layoutUID, unsigned instCount)
{
    const VertexLayout* vl = &vertexLayout;

    if (layoutUID != VertexLayout::InvalidUID)
    {
        bool do_add = true;

        for (std::vector<vdecl_t>::iterator i = altVdecl9.begin(), i_end = altVdecl9.end(); i != i_end; ++i)
        {
            if (i->layoutUID == layoutUID)
            {
                vl = &(i->layout);
                break;
            }
        }
    }
    // DO NOT try to add alt.vdecl here

    for (unsigned s = 0; s != vl->StreamCount(); ++s)
    {
        switch (vl->StreamFrequency(s))
        {
        case VDF_PER_VERTEX:
            _D3D9_Device->SetStreamSourceFreq(s, D3DSTREAMSOURCE_INDEXEDDATA | instCount);
            break;
        case VDF_PER_INSTANCE:
            _D3D9_Device->SetStreamSourceFreq(s, D3DSTREAMSOURCE_INSTANCEDATA | 1);
            break;
        }
    }
}

//------------------------------------------------------------------------------

bool PipelineStateDX9_t::FragmentProgDX9::Construct(const void* bin, unsigned bin_sz)
{
    bool success = false;
    LPD3DXBUFFER shader = NULL;
    LPD3DXBUFFER err = NULL;
    LPD3DXCONSTANTTABLE const_tab = NULL;
    HRESULT hr = D3DXCompileShader(
    (const char*)bin, bin_sz,
    NULL, // no defines
    NULL, // no include-interfaces
    "fp_main",
    "ps_3_0",
    0,
    &shader,
    &err,
    &const_tab);

    if (SUCCEEDED(hr))
    {
        void* code = shader->GetBufferPointer();
        DX9Command cmd = { DX9Command::CREATE_PIXEL_SHADER, { uint64_t((const DWORD*)code), uint64_t(&ps9) } };

        ExecDX9(&cmd, 1, false);

        if (SUCCEEDED(cmd.retval))
        {
            for (unsigned i = 0; i != MAX_CONST_BUFFER_COUNT; ++i)
            {
                char name[16];
                sprintf(name, "FP_Buffer%u", i);
                D3DXHANDLE c = const_tab->GetConstantByName(NULL, name);

                cbufElemCount[i] = 0;
                cbufRegBase[i] = DAVA::InvalidIndex;
                cbufRegCount[i] = 0;

                if (c)
                {
                    D3DXCONSTANT_DESC desc;
                    UINT cnt = 1;

                    hr = const_tab->GetConstantDesc(c, &desc, &cnt);

                    if (SUCCEEDED(hr))
                    {
                        cbufElemCount[i] = desc.Elements;
                        cbufRegBase[i] = desc.RegisterIndex;
                        cbufRegCount[i] = desc.RegisterCount;
                    }
                }
                else
                {
                    if (strstr((const char*)bin, name))
                    {
                        Logger::Warning("shader has \"%s\", but no variables actually use it in code:\n", name);
                        DumpShaderTextDX9((const char*)bin, bin_sz);
                    }
                }
            }

            // do some additional sanity checks
            for (unsigned i = 0; i != MAX_CONST_BUFFER_COUNT; ++i)
            {
                if (cbufRegBase[i] == DAVA::InvalidIndex)
                {
                    if (i == 0 && cbufRegBase[i + 1] != DAVA::InvalidIndex)
                    {
                        Logger::Warning("WARNING: fragment-const-buf [%u] is unused (all uniform/variables are unused)", i);
                    }
                }
            }

            success = true;
        }
        else
        {
            Logger::Error("FAILED to create pixel-shader:\n%s\n", D3D9ErrorText(hr));
        }
    }
    else
    {
        Logger::Error("FAILED to compile pixel-shader:\n");
        if (err)
        {
            Logger::Info((const char*)(err->GetBufferPointer()));
        }
        Logger::Error("fragment-shader text:\n");
        DumpShaderTextDX9((const char*)bin, bin_sz);
    }

    return success;
}

//------------------------------------------------------------------------------

Handle
PipelineStateDX9_t::FragmentProgDX9::CreateConstBuffer(unsigned buf_i)
{
    Handle handle = InvalidHandle;

    DVASSERT(buf_i < MAX_CONST_BUFFER_COUNT);

    if (buf_i < MAX_CONST_BUFFER_COUNT)
    {
        handle = ConstBufDX9Pool::Alloc();

        ConstBuf* cb = ConstBufDX9Pool::Get(handle);

        cb->Construct(PROG_FRAGMENT, cbufRegBase[buf_i], cbufRegCount[buf_i], cbufElemCount[buf_i]);
    }

    return handle;
}

//------------------------------------------------------------------------------

void PipelineStateDX9_t::FragmentProgDX9::SetToRHI()
{
    HRESULT hr = _D3D9_Device->SetPixelShader(ps9);

    if (FAILED(hr))
        Logger::Error("SetPixelShader failed:\n%s\n", D3D9ErrorText(hr));
}

//==============================================================================

static Handle
dx9_PipelineState_Create(const PipelineState::Descriptor& desc)
{
    Handle handle = PipelineStateDX9Pool::Alloc();
    PipelineStateDX9_t* ps = PipelineStateDX9Pool::Get(handle);
    bool vprog_valid = false;
    bool fprog_valid = false;
    const std::vector<uint8>& vprog_bin = rhi::ShaderCache::GetProg(desc.vprogUid);
    const std::vector<uint8>& fprog_bin = rhi::ShaderCache::GetProg(desc.fprogUid);

    //Logger::Info("create PS");
    //Logger::Info("  vprog= %s",desc.vprogUid.c_str());
    //Logger::Info("  fprog= %s",desc.vprogUid.c_str());
    //desc.vertexLayout.Dump();

    ps->vprog.uid = desc.vprogUid;
    ps->fprog.uid = desc.fprogUid;

    vprog_valid = ps->vprog.Construct((const char*)(&vprog_bin[0]), static_cast<unsigned>(vprog_bin.size()), desc.vertexLayout);
    fprog_valid = ps->fprog.Construct((const char*)(&fprog_bin[0]), static_cast<unsigned>(fprog_bin.size()));

    if (vprog_valid && fprog_valid)
    {
        ps->blendEnabled = desc.blending.rtBlend[0].blendEnabled;

        switch (desc.blending.rtBlend[0].colorSrc)
        {
        case BLENDOP_ZERO:
            ps->blendSrc = D3DBLEND_ZERO;
            break;
        case BLENDOP_ONE:
            ps->blendSrc = D3DBLEND_ONE;
            break;
        case BLENDOP_SRC_ALPHA:
            ps->blendSrc = D3DBLEND_SRCALPHA;
            break;
        case BLENDOP_INV_SRC_ALPHA:
            ps->blendSrc = D3DBLEND_INVSRCALPHA;
            break;
        case BLENDOP_SRC_COLOR:
            ps->blendSrc = D3DBLEND_SRCCOLOR;
            break;
        case BLENDOP_DST_COLOR:
            ps->blendSrc = D3DBLEND_DESTCOLOR;
            break;
        }

        switch (desc.blending.rtBlend[0].colorDst)
        {
        case BLENDOP_ZERO:
            ps->blendDst = D3DBLEND_ZERO;
            break;
        case BLENDOP_ONE:
            ps->blendDst = D3DBLEND_ONE;
            break;
        case BLENDOP_SRC_ALPHA:
            ps->blendDst = D3DBLEND_SRCALPHA;
            break;
        case BLENDOP_INV_SRC_ALPHA:
            ps->blendDst = D3DBLEND_INVSRCALPHA;
            break;
        case BLENDOP_SRC_COLOR:
            ps->blendDst = D3DBLEND_SRCCOLOR;
            break;
        case BLENDOP_DST_COLOR:
            ps->blendDst = D3DBLEND_DESTCOLOR;
            break;
        }
    }
    else
    {
        PipelineStateDX9Pool::Free(handle);
        handle = InvalidHandle;
    }

    ps->colorMask = 0;
    if (desc.blending.rtBlend[0].writeMask & COLORMASK_R)
        ps->colorMask |= D3DCOLORWRITEENABLE_RED;
    if (desc.blending.rtBlend[0].writeMask & COLORMASK_G)
        ps->colorMask |= D3DCOLORWRITEENABLE_GREEN;
    if (desc.blending.rtBlend[0].writeMask & COLORMASK_B)
        ps->colorMask |= D3DCOLORWRITEENABLE_BLUE;
    if (desc.blending.rtBlend[0].writeMask & COLORMASK_A)
        ps->colorMask |= D3DCOLORWRITEENABLE_ALPHA;

    return handle;
}

static void
dx9_PipelineState_Delete(Handle ps)
{
    PipelineStateDX9Pool::Free(ps);
}

//------------------------------------------------------------------------------

static Handle
dx9_PipelineState_CreateVertexConstBuffer(Handle ps, unsigned buf_i)
{
    PipelineStateDX9_t* ps9 = PipelineStateDX9Pool::Get(ps);

    return ps9->vprog.CreateConstBuffer(buf_i);
}

//------------------------------------------------------------------------------

static Handle
dx9_PipelineState_CreateFragmentConstBuffer(Handle ps, unsigned buf_i)
{
    PipelineStateDX9_t* ps9 = PipelineStateDX9Pool::Get(ps);

    return ps9->fprog.CreateConstBuffer(buf_i);
}

namespace PipelineStateDX9
{
void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_PipelineState_Create = &dx9_PipelineState_Create;
    dispatch->impl_PipelineState_Delete = &dx9_PipelineState_Delete;
    dispatch->impl_PipelineState_CreateVertexConstBuffer = &dx9_PipelineState_CreateVertexConstBuffer;
    dispatch->impl_PipelineState_CreateFragmentConstBuffer = &dx9_PipelineState_CreateFragmentConstBuffer;
}

void Init(uint32 maxCount)
{
    PipelineStateDX9Pool::Reserve(maxCount);
}

//------------------------------------------------------------------------------

void SetToRHI(Handle ps, uint32 layoutUID)
{
    PipelineStateDX9_t* ps9 = PipelineStateDX9Pool::Get(ps);

    ps9->vprog.SetToRHI(layoutUID, true);
    ps9->fprog.SetToRHI();

    if (ps9->blendEnabled)
    {
        _D3D9_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
        _D3D9_Device->SetRenderState(D3DRS_SRCBLEND, ps9->blendSrc);
        _D3D9_Device->SetRenderState(D3DRS_DESTBLEND, ps9->blendDst);
    }
    else
    {
        _D3D9_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
    }

    _D3D9_Device->SetRenderState(D3DRS_COLORWRITEENABLE, ps9->colorMask);
}

//------------------------------------------------------------------------------

void
SetupVertexStreams(Handle ps, uint32 layoutUID, uint32 instCount)
{
    PipelineStateDX9_t* ps9 = PipelineStateDX9Pool::Get(ps);

    ps9->vprog.SetupVertexStreams(layoutUID, instCount);
}

//------------------------------------------------------------------------------

unsigned
VertexLayoutStride(Handle ps, uint32 stream)
{
    PipelineStateDX9_t* ps9 = PipelineStateDX9Pool::Get(ps);

    return ps9->vprog.vertexLayout.Stride(stream);
}

} // namespace PipelineStateDX9

//------------------------------------------------------------------------------

static bool
dx9_ConstBuffer_SetConst(Handle cb, unsigned const_i, unsigned const_count, const float* data)
{
    PipelineStateDX9_t::ConstBuf* cb9 = ConstBufDX9Pool::Get(cb);

    return cb9->SetConst(const_i, const_count, data);
}

//------------------------------------------------------------------------------

static bool
dx9_ConstBuffer_SetConst1fv(Handle cb, unsigned const_i, unsigned const_sub_i, const float* data, uint32 dataCount)
{
    PipelineStateDX9_t::ConstBuf* cb9 = ConstBufDX9Pool::Get(cb);

    return cb9->SetConst(const_i, const_sub_i, data, dataCount);
}

//------------------------------------------------------------------------------

void dx9_ConstBuffer_Delete(Handle cb)
{
    PipelineStateDX9_t::ConstBuf* cb9 = ConstBufDX9Pool::Get(cb);

    cb9->Destroy();
    ConstBufDX9Pool::Free(cb);
}

//------------------------------------------------------------------------------

namespace ConstBufferDX9
{
void Init(uint32 maxCount)
{
    ConstBufDX9Pool::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_ConstBuffer_SetConst = &dx9_ConstBuffer_SetConst;
    dispatch->impl_ConstBuffer_SetConst1fv = &dx9_ConstBuffer_SetConst1fv;
    dispatch->impl_ConstBuffer_Delete = &dx9_ConstBuffer_Delete;
}

void InitializeRingBuffer(uint32 size)
{
    _DX9_DefConstRingBuf.Initialize(size);
}

const void* Instance(Handle cb)
{
    PipelineStateDX9_t::ConstBuf* cb9 = ConstBufDX9Pool::Get(cb);

    return cb9->InstData();
}

void SetToRHI(Handle cb, const void* instData)
{
    PipelineStateDX9_t::ConstBuf* cb9 = ConstBufDX9Pool::Get(cb);

    cb9->SetToRHI(instData);
}

void InvalidateAllConstBufferInstances()
{
    _DX9_DefConstRingBuf.Reset();

    for (ConstBufDX9Pool::Iterator b = ConstBufDX9Pool::Begin(), b_end = ConstBufDX9Pool::End(); b != b_end; ++b)
    {
        b->InvalidateInst();
    }
}
}

//==============================================================================
} // namespace rhi
