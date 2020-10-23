#include "rhi_DX11.h"
#include "../rhi_ShaderCache.h"
#include <D3D11Shader.h>
#include <D3Dcompiler.h>

namespace rhi
{
struct PipelineStateDX11_t
{
    struct LayoutInfo
    {
        ID3D11InputLayout* inputLayout = nullptr;
        uint32 layoutUID = 0;
        LayoutInfo(ID3D11InputLayout* i, uint32 uid);
    };
    PipelineState::Descriptor desc;

    ID3D11InputLayout* inputLayout = nullptr;
    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11PixelShader* pixelShader = nullptr;
    ID3D11BlendState* blendState = nullptr;
    ID3D10Blob* vpCode = nullptr;

    uint32 vertexBufCount = 0;
    uint32 vertexBufRegCount[16];
    uint32 fragmentBufCount = 0;
    uint32 fragmentBufRegCount[16];

    VertexLayout vertexLayout;
    DAVA::Vector<LayoutInfo> altLayout;
    DAVA::Vector<uint8> dbgVertexSrc;
    DAVA::Vector<uint8> dbgPixelSrc;

    Handle CreateConstBuffer(ProgType type, uint32 buf_i);
    static ID3D11InputLayout* CreateInputLayout(const VertexLayout& layout, const void* code, uint32 code_sz);
    static ID3D11InputLayout* CreateCompatibleInputLayout(const VertexLayout& vbLayout, const VertexLayout& vprogLayout, const void* code, uint32 code_sz);
};

using PipelineStateDX11Pool = ResourcePool<PipelineStateDX11_t, RESOURCE_PIPELINE_STATE, PipelineState::Descriptor, false>;
RHI_IMPL_POOL(PipelineStateDX11_t, RESOURCE_PIPELINE_STATE, PipelineState::Descriptor, false);

PipelineStateDX11_t::LayoutInfo::LayoutInfo(ID3D11InputLayout* i, uint32 uid)
    : inputLayout(i)
    , layoutUID(uid)
{
}

ID3D11InputLayout* PipelineStateDX11_t::CreateInputLayout(const VertexLayout& layout, const void* code, uint32 code_sz)
{
    ID3D11InputLayout* vdecl = nullptr;
    D3D11_INPUT_ELEMENT_DESC elem[32];
    uint32 elemCount = 0;

    DVASSERT(layout.ElementCount() < countof(elem));
    for (uint32 i = 0; i != layout.ElementCount(); ++i)
    {
        if (layout.ElementSemantics(i) == VS_PAD)
            continue;

        uint32 stream_i = layout.ElementStreamIndex(i);

        elem[elemCount].AlignedByteOffset = (UINT)(layout.ElementOffset(i));
        elem[elemCount].SemanticIndex = layout.ElementSemanticsIndex(i);
        elem[elemCount].InputSlot = stream_i;

        if (layout.StreamFrequency(stream_i) == VDF_PER_INSTANCE)
        {
            elem[elemCount].InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
            elem[elemCount].InstanceDataStepRate = 1;
        }
        else
        {
            elem[elemCount].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            elem[elemCount].InstanceDataStepRate = 0;
        }

        switch (layout.ElementSemantics(i))
        {
        case VS_POSITION:
        {
            elem[elemCount].SemanticName = "POSITION";
        }
        break;

        case VS_NORMAL:
        {
            elem[elemCount].SemanticName = "NORMAL";
        }
        break;

        case VS_COLOR:
        {
            elem[elemCount].SemanticName = "COLOR";
        }
        break;

        case VS_TEXCOORD:
        {
            elem[elemCount].SemanticName = "TEXCOORD";
        }
        break;

        case VS_TANGENT:
        {
            elem[elemCount].SemanticName = "TANGENT";
        }
        break;

        case VS_BINORMAL:
        {
            elem[elemCount].SemanticName = "BINORMAL";
        }
        break;

        case VS_BLENDWEIGHT:
        {
            elem[elemCount].SemanticName = "BLENDWEIGHT";
        }
        break;

        case VS_BLENDINDEX:
        {
            elem[elemCount].SemanticName = "BLENDINDICES";
        }
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
                elem[elemCount].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                break;
            case 3:
                elem[elemCount].Format = DXGI_FORMAT_R32G32B32_FLOAT;
                break;
            case 2:
                elem[elemCount].Format = DXGI_FORMAT_R32G32_FLOAT;
                break;
            case 1:
                elem[elemCount].Format = DXGI_FORMAT_R32_FLOAT;
                break;
            }
        }
        break;
        default:
            break;
        }

        if (layout.ElementSemantics(i) == VS_COLOR)
        {
            elem[elemCount].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        }
        ++elemCount;
    }

    DX11DeviceCommand(DX11Command::CREATE_INPUT_LAYOUT, elem, elemCount, code, code_sz, &vdecl);

    return vdecl;
}

ID3D11InputLayout* PipelineStateDX11_t::CreateCompatibleInputLayout(const VertexLayout& vbLayout, const VertexLayout& vprogLayout, const void* code, uint32 code_sz)
{
    ID3D11InputLayout* vdecl = nullptr;
    D3D11_INPUT_ELEMENT_DESC elem[32] = {};
    uint32 elemCount = 0;

    DVASSERT(vbLayout.ElementCount() < countof(elem));
    for (uint32 i = 0; i != vprogLayout.ElementCount(); ++i)
    {
        DVASSERT(vprogLayout.ElementSemantics(i) != VS_PAD);

        uint32 vb_elem_i = DAVA::InvalidIndex;

        for (uint32 k = 0; k != vbLayout.ElementCount(); ++k)
        {
            if (vbLayout.ElementSemantics(k) == vprogLayout.ElementSemantics(i) && vbLayout.ElementSemanticsIndex(k) == vprogLayout.ElementSemanticsIndex(i))
            {
                vb_elem_i = k;
                break;
            }
        }

        if (vb_elem_i != DAVA::InvalidIndex)
        {
            uint32 stream_i = vprogLayout.ElementStreamIndex(i);

            elem[elemCount].AlignedByteOffset = (UINT)(vbLayout.ElementOffset(vb_elem_i));
            elem[elemCount].SemanticIndex = vprogLayout.ElementSemanticsIndex(i);
            elem[elemCount].InputSlot = stream_i;

            if (vprogLayout.StreamFrequency(stream_i) == VDF_PER_INSTANCE)
            {
                elem[elemCount].InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
                elem[elemCount].InstanceDataStepRate = 1;
            }
            else
            {
                elem[elemCount].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                elem[elemCount].InstanceDataStepRate = 0;
            }

            switch (vbLayout.ElementSemantics(vb_elem_i))
            {
            case VS_POSITION:
            {
                elem[elemCount].SemanticName = "POSITION";
            }
            break;

            case VS_NORMAL:
            {
                elem[elemCount].SemanticName = "NORMAL";
            }
            break;

            case VS_COLOR:
            {
                elem[elemCount].SemanticName = "COLOR";
            }
            break;

            case VS_TEXCOORD:
            {
                elem[elemCount].SemanticName = "TEXCOORD";
            }
            break;

            case VS_TANGENT:
            {
                elem[elemCount].SemanticName = "TANGENT";
            }
            break;

            case VS_BINORMAL:
            {
                elem[elemCount].SemanticName = "BINORMAL";
            }
            break;

            case VS_BLENDWEIGHT:
            {
                elem[elemCount].SemanticName = "BLENDWEIGHT";
            }
            break;

            case VS_BLENDINDEX:
            {
                elem[elemCount].SemanticName = "BLENDINDICES";
            }
            break;
            default:
                break;
            }

            switch (vbLayout.ElementDataType(vb_elem_i))
            {
            case VDT_FLOAT:
            {
                switch (vbLayout.ElementDataCount(vb_elem_i))
                {
                case 4:
                    elem[elemCount].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                    break;
                case 3:
                    elem[elemCount].Format = DXGI_FORMAT_R32G32B32_FLOAT;
                    break;
                case 2:
                    elem[elemCount].Format = DXGI_FORMAT_R32G32_FLOAT;
                    break;
                case 1:
                    elem[elemCount].Format = DXGI_FORMAT_R32_FLOAT;
                    break;
                }
            }
            break;
            default:
                break;
            }

            if (vbLayout.ElementSemantics(vb_elem_i) == VS_COLOR)
            {
                elem[elemCount].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            }
        }
        else
        {
            DAVA::Logger::Error("Incompatible vertex layout. Missing element %s%d of type %s", VertexSemanticsName(vprogLayout.ElementSemantics(i)),
                                vprogLayout.ElementSemanticsIndex(i), VertexDataTypeName(vprogLayout.ElementDataType(i)));

            DAVA::Logger::Error("Program:");
            vprogLayout.Dump();

            DAVA::Logger::Error("VertexBuffer:");
            vbLayout.Dump();

            DVASSERT(!"kaboom!");
        }

        ++elemCount;
    }

    if (vprogLayout.Stride() < vbLayout.Stride())
    {
        const uint32 padCnt = vbLayout.Stride() - vprogLayout.Stride();

        DVASSERT(padCnt % 4 == 0);
        for (uint32 p = 0; p != padCnt / 4; ++p)
        {
            elem[elemCount].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT; //vprogLayout.Stride() + p;
            elem[elemCount].SemanticIndex = p;
            elem[elemCount].SemanticName = "PAD";
            elem[elemCount].InputSlot = 0;
            elem[elemCount].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            elem[elemCount].InstanceDataStepRate = 0;
            elem[elemCount].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            ++elemCount;
        }
    }

    DX11DeviceCommand(DX11Command::CREATE_INPUT_LAYOUT, elem, elemCount, code, code_sz, &vdecl);

    return vdecl;
}

Handle PipelineStateDX11_t::CreateConstBuffer(ProgType type, uint32 buf_i)
{
    return ConstBufferDX11::Alloc(type, buf_i, (type == PROG_VERTEX) ? vertexBufRegCount[buf_i] : fragmentBufRegCount[buf_i]);
}

static void DumpShaderText(const char* code, uint32 code_sz)
{
    char src[64 * 1024] = {};
    if (code_sz + 1 >= sizeof(src))
    {
        DAVA::Logger::Info(code);
        return;
    }

    char* src_line[1024] = {};
    uint32 line_cnt = 0;
    memcpy(src, code, code_sz);

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

    for (uint32 i = 0; i != line_cnt; ++i)
    {
        DAVA::Logger::Info("%4u |  %s", 1 + i, src_line[i]);
    }
}

static Handle dx11_PipelineState_Create(const PipelineState::Descriptor& desc)
{
    bool success = false;
    Handle handle = PipelineStateDX11Pool::Alloc();
    PipelineStateDX11_t* ps = PipelineStateDX11Pool::Get(handle);
    ID3D10Blob* vp_code = nullptr;
    ID3D10Blob* vp_err = nullptr;
    ID3D10Blob* fp_code = nullptr;
    ID3D10Blob* fp_err = nullptr;

#if 0
    DAVA::Logger::Info("create PS");
    DAVA::Logger::Info("  vprog= %s", desc.vprogUid.c_str());
    DAVA::Logger::Info("  fprog= %s", desc.vprogUid.c_str());
    desc.vertexLayout.Dump();
#endif

    const std::vector<uint8>& vprog_bin = rhi::ShaderCache::GetProg(desc.vprogUid);
    const std::vector<uint8>& fprog_bin = rhi::ShaderCache::GetProg(desc.fprogUid);

#if 0
    DumpShaderText((const char*)(&vprog_bin[0]), (uint32)vprog_bin.size());
    DumpShaderText((const char*)(&fprog_bin[0]), (uint32)fprog_bin.size());
#endif

    const char* vsFeatureLevel = (dx11.usedFeatureLevel >= D3D_FEATURE_LEVEL_10_0) ? "vs_4_0" : "vs_4_0_level_9_1";
    const char* fsFeatureLevel = (dx11.usedFeatureLevel >= D3D_FEATURE_LEVEL_10_0) ? "ps_4_0" : "ps_4_0_level_9_1";

    HRESULT hr = D3DCompile((const char*)(&vprog_bin[0]), vprog_bin.size(), "vprog", nullptr, nullptr, "vp_main", vsFeatureLevel,
                            D3DCOMPILE_OPTIMIZATION_LEVEL2, 0, &vp_code, &vp_err);

    if (DX11Check(hr))
    {
        if (DX11DeviceCommand(DX11Command::CREATE_VERTEX_SHADER, vp_code->GetBufferPointer(), vp_code->GetBufferSize(), NULL, &(ps->vertexShader)))
        {
            ID3D11ShaderReflection* reflection = nullptr;
            hr = D3DReflect(vp_code->GetBufferPointer(), vp_code->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&reflection);
            if (DX11Check(hr))
            {
                D3D11_SHADER_DESC desc = {};
                hr = reflection->GetDesc(&desc);
                if (DX11Check(hr))
                {
                    ps->vertexBufCount = desc.ConstantBuffers;

                    for (uint32 b = 0; b != desc.ConstantBuffers; ++b)
                    {
                        ID3D11ShaderReflectionConstantBuffer* cb = reflection->GetConstantBufferByIndex(b);
                        if (cb)
                        {
                            D3D11_SHADER_BUFFER_DESC cb_desc = {};
                            hr = cb->GetDesc(&cb_desc);
                            if (DX11Check(hr))
                            {
                                ps->vertexBufRegCount[b] = cb_desc.Size / (4 * sizeof(float));
                            }
                        }
                    }
                }
            }
        }
        else
        {
            ps->vertexShader = nullptr;
        }
    }
    else
    {
        DAVA::Logger::Error("FAILED to compile vertex-shader:");
        if (vp_err)
        {
            DAVA::Logger::Info((const char*)(vp_err->GetBufferPointer()));
        }
        DAVA::Logger::Error("shader-uid : %s", desc.vprogUid.c_str());
        DAVA::Logger::Error("vertex-shader text:\n");
        DumpShaderText((const char*)(&vprog_bin[0]), (uint32)vprog_bin.size());
        ps->vertexShader = nullptr;
        DVASSERT(ps->vertexShader, desc.vprogUid.c_str());
    }

    hr = D3DCompile((const char*)(&fprog_bin[0]), fprog_bin.size(), "fprog", nullptr, nullptr, "fp_main", fsFeatureLevel,
                    D3DCOMPILE_OPTIMIZATION_LEVEL2, 0, &fp_code, &fp_err);

    if (DX11Check(hr))
    {
        if (DX11DeviceCommand(DX11Command::CREATE_PIXEL_SHADER, fp_code->GetBufferPointer(), fp_code->GetBufferSize(), NULL, &(ps->pixelShader)))
        {
            ID3D11ShaderReflection* reflection = nullptr;
            hr = D3DReflect(fp_code->GetBufferPointer(), fp_code->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&reflection);
            if (DX11Check(hr))
            {
                D3D11_SHADER_DESC desc = {};
                hr = reflection->GetDesc(&desc);
                if (DX11Check(hr))
                {
                    ps->fragmentBufCount = desc.ConstantBuffers;
                    for (uint32 b = 0; b != desc.ConstantBuffers; ++b)
                    {
                        ID3D11ShaderReflectionConstantBuffer* cb = reflection->GetConstantBufferByIndex(b);
                        if (cb)
                        {
                            D3D11_SHADER_BUFFER_DESC cb_desc = {};
                            hr = cb->GetDesc(&cb_desc);
                            if (DX11Check(hr))
                            {
                                ps->fragmentBufRegCount[b] = cb_desc.Size / (4 * sizeof(float));
                            }
                        }
                    }
                }
            }
        }
        else
        {
            ps->pixelShader = nullptr;
            DVASSERT(ps->pixelShader, desc.fprogUid.c_str());
        }
    }
    else
    {
        DAVA::Logger::Error("FAILED to compile pixel-shader:");
        if (fp_err)
        {
            DAVA::Logger::Info((const char*)(fp_err->GetBufferPointer()));
        }
        DAVA::Logger::Error("shader-uid : %s", desc.fprogUid.c_str());
        DAVA::Logger::Error("vertex-shader text:\n");
        DumpShaderText((const char*)(&fprog_bin[0]), (uint32)fprog_bin.size());
        ps->pixelShader = nullptr;
    }

    if (ps->vertexShader && ps->pixelShader)
    {
        ps->vpCode = vp_code;
        ps->inputLayout = PipelineStateDX11_t::CreateInputLayout(desc.vertexLayout, vp_code->GetBufferPointer(), static_cast<uint32>(vp_code->GetBufferSize()));
        ps->vertexLayout = desc.vertexLayout;
        DVASSERT(ps->inputLayout);

        if (ps->inputLayout)
        {
            ps->dbgVertexSrc = vprog_bin;
            ps->dbgPixelSrc = fprog_bin;

            UINT8 mask = 0;

            if (desc.blending.rtBlend[0].writeMask & COLORMASK_R)
                mask |= D3D11_COLOR_WRITE_ENABLE_RED;
            if (desc.blending.rtBlend[0].writeMask & COLORMASK_G)
                mask |= D3D11_COLOR_WRITE_ENABLE_GREEN;
            if (desc.blending.rtBlend[0].writeMask & COLORMASK_B)
                mask |= D3D11_COLOR_WRITE_ENABLE_BLUE;
            if (desc.blending.rtBlend[0].writeMask & COLORMASK_A)
                mask |= D3D11_COLOR_WRITE_ENABLE_ALPHA;

            D3D11_BLEND_DESC bs_desc = {};
            bs_desc.AlphaToCoverageEnable = FALSE;
            bs_desc.IndependentBlendEnable = FALSE;
            bs_desc.RenderTarget[0].BlendEnable = desc.blending.rtBlend[0].blendEnabled;
            bs_desc.RenderTarget[0].RenderTargetWriteMask = mask;
            bs_desc.RenderTarget[0].SrcBlend = DX11_BlendOp(BlendOp(desc.blending.rtBlend[0].colorSrc));
            bs_desc.RenderTarget[0].DestBlend = DX11_BlendOp(BlendOp(desc.blending.rtBlend[0].colorDst));
            bs_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            bs_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            bs_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
            bs_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            if (DX11DeviceCommand(DX11Command::CREATE_BLEND_STATE, &bs_desc, &ps->blendState))
            {
                ps->desc = desc;
                success = true;
            }
        }
    }

    if (!success)
    {
        PipelineStateDX11Pool::Free(handle);
        handle = InvalidHandle;
    }

    return handle;
}

static void dx11_PipelineState_Delete(Handle ps)
{
    PipelineStateDX11Pool::Free(ps);
}

static Handle dx11_PipelineState_CreateVertexConstBuffer(Handle ps, uint32 buf_i)
{
    PipelineStateDX11_t* ps11 = PipelineStateDX11Pool::Get(ps);
    return ps11->CreateConstBuffer(PROG_VERTEX, buf_i);
}

static Handle dx11_PipelineState_CreateFragmentConstBuffer(Handle ps, uint32 buf_i)
{
    PipelineStateDX11_t* ps11 = PipelineStateDX11Pool::Get(ps);
    return ps11->CreateConstBuffer(PROG_FRAGMENT, buf_i);
}

void PipelineStateDX11::Init(uint32 maxCount)
{
    PipelineStateDX11Pool::Reserve(maxCount);
}

void PipelineStateDX11::SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_PipelineState_Create = &dx11_PipelineState_Create;
    dispatch->impl_PipelineState_Delete = &dx11_PipelineState_Delete;
    dispatch->impl_PipelineState_CreateVertexConstBuffer = &dx11_PipelineState_CreateVertexConstBuffer;
    dispatch->impl_PipelineState_CreateFragmentConstBuffer = &dx11_PipelineState_CreateFragmentConstBuffer;
}

void PipelineStateDX11::SetToRHI(Handle ps, uint32 layoutUID, ID3D11DeviceContext* context)
{
    PipelineStateDX11_t* ps11 = PipelineStateDX11Pool::Get(ps);

    ID3D11InputLayout* layout11 = nullptr;
    if (layoutUID == VertexLayout::InvalidUID)
    {
        layout11 = ps11->inputLayout;
    }
    else
    {
        for (const PipelineStateDX11_t::LayoutInfo& l : ps11->altLayout)
        {
            if (l.layoutUID == layoutUID)
            {
                layout11 = l.inputLayout;
                break;
            }
        }

        if (layout11 == nullptr)
        {
            const VertexLayout* vbLayout = VertexLayout::Get(layoutUID);
            uint32 bufferSize = static_cast<uint32>(ps11->vpCode->GetBufferSize());
            layout11 = PipelineStateDX11_t::CreateCompatibleInputLayout(*vbLayout, ps11->vertexLayout, ps11->vpCode->GetBufferPointer(), bufferSize);

            if (layout11)
            {
                ps11->altLayout.emplace_back(layout11, layoutUID);
            }
            else
            {
                DAVA::Logger::Error("Unable to compatible vertex-layout");
                DAVA::Logger::Info("vprog-layout:");
                ps11->vertexLayout.Dump();
                DAVA::Logger::Info("custom-layout:");
                vbLayout->Dump();
            }
        }
    }

    DVASSERT(layout11 != nullptr);
    context->IASetInputLayout(layout11);

    DVASSERT(ps11->vertexShader != nullptr);
    context->VSSetShader(ps11->vertexShader, nullptr, 0);

    DVASSERT(ps11->pixelShader != nullptr);
    context->PSSetShader(ps11->pixelShader, nullptr, 0);

    DVASSERT(ps11->blendState != nullptr);
    context->OMSetBlendState(ps11->blendState, nullptr, 0xFFFFFFFF);
}

uint32 PipelineStateDX11::VertexLayoutStride(Handle ps, uint32 stream_i)
{
    PipelineStateDX11_t* ps11 = PipelineStateDX11Pool::Get(ps);
    return ps11->vertexLayout.Stride(stream_i);
}

uint32 PipelineStateDX11::VertexLayoutStreamCount(Handle ps)
{
    PipelineStateDX11_t* ps11 = PipelineStateDX11Pool::Get(ps);
    return ps11->vertexLayout.StreamCount();
}

void PipelineStateDX11::GetConstBufferCount(Handle ps, uint32* vertexBufCount, uint32* fragmentBufCount)
{
    PipelineStateDX11_t* ps11 = PipelineStateDX11Pool::Get(ps);
    *vertexBufCount = ps11->vertexBufCount;
    *fragmentBufCount = ps11->fragmentBufCount;
}

} // namespace rhi
