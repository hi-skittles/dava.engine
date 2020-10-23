#include "Debug/Private/ImGui.h"

#include "Base/BaseTypes.h"
#include "Debug/DVAssert.h"
#include "Debug/Private/ImGuiUtils.h"
#include "DeviceManager/DeviceManager.h"
#include "Engine/Engine.h"
#include "Input/InputEvent.h"
#include "Input/InputSystem.h"
#include "Input/Mouse.h"
#include "Input/Keyboard.h"
#include "Input/TouchScreen.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/DynamicBufferAllocator.h"
#include "Render/RenderBase.h"
#include "Render/Renderer.h"
#include "Render/RHI/rhi_Public.h"
#include "Render/RHI/rhi_ShaderCache.h"
#include "Render/RHI/rhi_ShaderSource.h"
#include "Time/SystemTimer.h"
#include "UI/UIControlSystem.h"
#include "Utils/Utils.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"

#include <imgui/imgui_internal.h>

namespace ImGuiImplDetails
{
static bool initialized = false;

static rhi::HPipelineState pipelineStatePC, pipelineStatePTC;
static rhi::HConstBuffer constBufferPC, constBufferPTC;
static rhi::HDepthStencilState depthState;
static DAVA::uint32 vertexLayout = 0;
static rhi::HSamplerState fontSamplerState;
static rhi::HTextureSet fontTextureSet;
static rhi::HTexture fontTexture;

static DAVA::TrackedObject* trackedObject = nullptr;
static DAVA::uint32 inputHandlerToken = 0;

static DAVA::Size2i framebufferSize = { 0, 0 };

static const char* IMGUI_RENDER_PASS_MARKER_NAME = "ImGuiRenderPass";

static const char* vprogPC =
"vertex_in\n"
"{\n"
"    float2 pos : TEXCOORD0;\n"
"    float4 color : COLOR0;\n"
"};\n"
"vertex_out\n"
"{\n"
"    float4 position : SV_POSITION;\n"
"    float4 color : COLOR0;\n"
"};\n"
"[material][unique] property float4x4 XForm;"
"vertex_out vp_main( vertex_in input )\n"
"{\n"
"    vertex_out output;\n"
"    output.position = mul( float4(input.pos.x,-input.pos.y,0.0,1.0), XForm );\n"
"    output.color = input.color;\n"
"    return output;\n"
"}\n";

static const char* fprogPC =
"fragment_in\n"
"{\n"
"    float4 color : COLOR0;\n"
"};\n"
"fragment_out\n"
"{\n"
"    float4 color : SV_TARGET0;\n"
"};\n"
"\n"
"fragment_out\n"
"fp_main( fragment_in input )\n"
"{\n"
"    fragment_out output;\n"
"    output.color = input.color;\n"
"    return output;\n"
"}\n"
"blending { src=src_alpha dst=inv_src_alpha }\n";

static const char* vprogPTC =
"vertex_in\n"
"{\n"
"    float2 pos : TEXCOORD0;\n"
"    float2 uv : TEXCOORD1;\n"
"    float4 color : COLOR0;\n"
"};\n"
"vertex_out\n"
"{\n"
"    float4 position : SV_POSITION;\n"
"    float2 uv : TEXCOORD0;\n"
"    float4 color : COLOR0;\n"
"};\n"
"[material][unique] property float4x4 XForm;"
"vertex_out vp_main( vertex_in input )\n"
"{\n"
"    vertex_out output;\n"
"    output.position = mul(float4(input.pos.x,-input.pos.y,0.0,1.0), XForm);\n"
"    output.uv = input.uv;\n"
"    output.color = input.color;\n"
"    return output;\n"
"}\n";

static const char* fprogPTC =
"fragment_in\n"
"{\n"
"    float2 uv    : TEXCOORD0;\n"
"    float4 color : COLOR0;\n"
"};\n"
"fragment_out\n"
"{\n"
"    float4 color : SV_TARGET0;\n"
"};\n"
"\n"
"uniform sampler2D Image;\n"
"\n"
"fragment_out\n"
"fp_main( fragment_in input )\n"
"{\n"
"    fragment_out output;\n"
"    float4       image = tex2D( Image, input.uv );"
"    output.color = image * input.color;\n"
"    return output;\n"
"}\n"
"blending { src=src_alpha dst=inv_src_alpha }\n";

// Pair of buffers and sync object to use for draw calls inside of ImGuiDrawFn
struct DynamicBuffers
{
    rhi::HVertexBuffer vbuffer;
    DAVA::uint32 vbufferSize;

    rhi::HIndexBuffer ibuffer;
    DAVA::uint32 ibufferSize;

    rhi::HSyncObject syncObject;
};

// List of buffers currently used by GPU
static DAVA::Vector<DynamicBuffers*> dynamicBuffersUsed;

// List of buffers which are not used by GPU
static DAVA::Vector<DynamicBuffers*> dynamicBuffersIdle;

void CreateVertexBuffer(DynamicBuffers* buffers, DAVA::uint32 size)
{
    DVASSERT(buffers != nullptr);
    DVASSERT(size > 0);

    rhi::VertexBuffer::Descriptor vertexBufferDesc;
    vertexBufferDesc.size = size;
    vertexBufferDesc.usage = rhi::USAGE_DYNAMICDRAW;
    vertexBufferDesc.needRestore = false;

    buffers->vbuffer = rhi::CreateVertexBuffer(vertexBufferDesc);
    buffers->vbufferSize = vertexBufferDesc.size;
}

void CreateIndexBuffer(DynamicBuffers* buffers, DAVA::uint32 size)
{
    DVASSERT(buffers != nullptr);
    DVASSERT(size > 0);

    rhi::IndexBuffer::Descriptor indexBufferDesc;
    indexBufferDesc.size = size;
    indexBufferDesc.usage = rhi::USAGE_DYNAMICDRAW;
    indexBufferDesc.needRestore = false;

    buffers->ibuffer = rhi::CreateIndexBuffer(indexBufferDesc);
    buffers->ibufferSize = indexBufferDesc.size;
}

DynamicBuffers* CreateDynamicBuffers(DAVA::uint32 vbufferSize, DAVA::uint32 ibufferSize)
{
    DVASSERT(vbufferSize > 0);
    DVASSERT(ibufferSize > 0);

    DynamicBuffers* dynamicBuffers = new DynamicBuffers();
    CreateVertexBuffer(dynamicBuffers, vbufferSize);
    CreateIndexBuffer(dynamicBuffers, ibufferSize);

    return dynamicBuffers;
}

void DeleteDynamicBuffers(DynamicBuffers* buffers)
{
    DVASSERT(buffers != nullptr);

    rhi::DeleteVertexBuffer(buffers->vbuffer);
    rhi::DeleteIndexBuffer(buffers->ibuffer);
    delete buffers;
}

void ResizeBuffersIfNeeded(DynamicBuffers* buffers, DAVA::uint32 vbufferSize, DAVA::uint32 ibufferSize)
{
    DVASSERT(buffers != nullptr);
    DVASSERT(vbufferSize > 0);
    DVASSERT(ibufferSize > 0);

    if (buffers->vbufferSize < vbufferSize)
    {
        rhi::DeleteVertexBuffer(buffers->vbuffer);
        CreateVertexBuffer(buffers, vbufferSize);
    }

    if (buffers->ibufferSize < ibufferSize)
    {
        rhi::DeleteIndexBuffer(buffers->ibuffer);
        CreateIndexBuffer(buffers, ibufferSize);
    }
}

void ImGuiDrawFn(ImDrawData* data)
{
    using namespace DAVA;

    if (!data->Valid)
        return;

    const float32 scale = ImGuiUtils::GetScale() * ImGuiUtils::GetImGuiScreenToPhysicalScreenSizeScale();

    Vector4 rect = { 0.f, 0.f, static_cast<float32>(framebufferSize.dx), static_cast<float32>(framebufferSize.dy) };
    rect /= scale;

    VirtualCoordinatesSystem* vcs = GetEngineContext()->uiControlSystem->vcs;
    Vector2 offset = vcs->GetPhysicalDrawOffset();

    rhi::RenderPassConfig passConfig;
    passConfig.colorBuffer[0].loadAction = rhi::LOADACTION_LOAD;
    passConfig.colorBuffer[0].storeAction = rhi::STOREACTION_STORE;
    passConfig.depthStencilBuffer.loadAction = rhi::LOADACTION_NONE;
    passConfig.depthStencilBuffer.storeAction = rhi::STOREACTION_NONE;
    passConfig.priority = PRIORITY_MAIN_2D - 10;
    passConfig.viewport.x = static_cast<uint32>(offset.x);
    passConfig.viewport.y = static_cast<uint32>(offset.y);
    passConfig.viewport.width = framebufferSize.dx;
    passConfig.viewport.height = framebufferSize.dy;

    Matrix4 ortho(
    2.0f / rect.z, 0.0f, 0.0f, -1.0f,
    0.0f, 2.0f / rect.w, 0.0f, 1.0f,
    0.0f, 0.0f, -1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f);

    if (rhi::DeviceCaps().isCenterPixelMapping)
    {
        ortho._03 -= 1.0f / rect.z;
        ortho._13 -= 1.0f / rect.w;
    }

    ortho.Transpose();

    rhi::UpdateConstBuffer4fv(constBufferPC, 0, ortho.data, 4);
    rhi::UpdateConstBuffer4fv(constBufferPTC, 0, ortho.data, 4);

    rhi::HPacketList packetList;
    rhi::HRenderPass pass = rhi::AllocateRenderPass(passConfig, 1, &packetList);
    rhi::BeginRenderPass(pass);
    rhi::BeginPacketList(packetList);

    for (int32 i = 0; i < data->CmdListsCount; ++i)
    {
        ImDrawList* cmdList = data->CmdLists[i];

        DVASSERT(cmdList->IdxBuffer.size() % 3 == 0);

        const uint32 vbufferSize = cmdList->VtxBuffer.size() * sizeof(ImDrawVert);
        const uint32 ibufferSize = cmdList->IdxBuffer.size() * sizeof(uint16);

        // DynamicBufferAllocator is not used to avoid dealing with splitting vertex and index buffers due to lack of space

        // Move currently not used buffers from dynamicBuffersUsed to dynamicBuffersIdle
        for (uint32 usedBuffersIndex = 0; usedBuffersIndex < dynamicBuffersUsed.size(); ++usedBuffersIndex)
        {
            DynamicBuffers* buffers = dynamicBuffersUsed[usedBuffersIndex];
            DVASSERT(buffers != nullptr);

            if (rhi::SyncObjectSignaled(buffers->syncObject))
            {
                RemoveExchangingWithLast(dynamicBuffersUsed, usedBuffersIndex);
                dynamicBuffersIdle.push_back(buffers);
            }
        }

        // Get buffers to use for this call
        DynamicBuffers* buffers = nullptr;
        if (dynamicBuffersIdle.size() > 0)
        {
            // If there are not used objects, get it and make new data fit
            buffers = dynamicBuffersIdle.back();
            dynamicBuffersIdle.pop_back();
            ResizeBuffersIfNeeded(buffers, vbufferSize, ibufferSize);
        }
        else
        {
            // Otherwise create a new one
            buffers = CreateDynamicBuffers(vbufferSize, ibufferSize);
        }

        DVASSERT(buffers != nullptr);

        // Mark as used and remember fence
        dynamicBuffersUsed.push_back(buffers);
        buffers->syncObject = rhi::GetCurrentFrameSyncObject();

        // Copy data

        void* vertexBufferMappedData = rhi::MapVertexBuffer(buffers->vbuffer, 0, vbufferSize);
        if (rhi::HostApi() == rhi::RHI_DX9)
        {
            ImDrawVert* vxPtr = reinterpret_cast<ImDrawVert*>(vertexBufferMappedData);
            for (ImDrawVert& vx : cmdList->VtxBuffer)
            {
                vxPtr->pos = vx.pos;
                vxPtr->uv = vx.uv;
                vxPtr->col = rhi::NativeColorRGBA(vx.col);
                ++vxPtr;
            }
        }
        else
        {
            Memcpy(vertexBufferMappedData, cmdList->VtxBuffer.Data, vbufferSize);
        }
        rhi::UnmapVertexBuffer(buffers->vbuffer);

        void* indexBufferMappedData = rhi::MapIndexBuffer(buffers->ibuffer, 0, ibufferSize);
        Memcpy(indexBufferMappedData, cmdList->IdxBuffer.Data, ibufferSize);
        rhi::UnmapIndexBuffer(buffers->ibuffer);

        // Create rhi packets

        rhi::Packet packet;
        packet.vertexStreamCount = 1;
        packet.vertexStream[0] = buffers->vbuffer;
        packet.vertexCount = cmdList->VtxBuffer.size();
        packet.baseVertex = 0;
        packet.startIndex = 0;
        packet.indexBuffer = buffers->ibuffer;
        packet.cullMode = rhi::CULL_NONE;
        packet.depthStencilState = depthState;
        packet.vertexConstCount = 1;
        packet.fragmentConstCount = 0;
        packet.primitiveType = rhi::PRIMITIVE_TRIANGLELIST;
        packet.vertexLayoutUID = ImGuiImplDetails::vertexLayout;

        for (ImDrawCmd& cmd : cmdList->CmdBuffer)
        {
            if (cmd.TextureId)
            {
                packet.renderPipelineState = pipelineStatePTC;
                packet.vertexConst[0] = constBufferPTC;

                packet.textureSet = *reinterpret_cast<rhi::HTextureSet*>(cmd.TextureId);
                packet.samplerState = fontSamplerState;
            }
            else
            {
                packet.renderPipelineState = pipelineStatePC;
                packet.vertexConst[0] = constBufferPC;
                packet.textureSet = rhi::HTextureSet();
                packet.samplerState = rhi::HSamplerState();
            }

            packet.primitiveCount = cmd.ElemCount / 3;

            auto& cr = cmd.ClipRect;

            Vector4 v = { cr.x, cr.y, cr.z, cr.w };
            v *= scale;

            cmd.ClipRect = { v.x + offset.x, v.y + offset.y, v.z + offset.x, v.w + offset.y };

            packet.scissorRect.x = static_cast<uint16>(cmd.ClipRect.x);
            packet.scissorRect.y = static_cast<uint16>(cmd.ClipRect.y);
            packet.scissorRect.width = static_cast<uint16>(cmd.ClipRect.z - cmd.ClipRect.x);
            packet.scissorRect.height = static_cast<uint16>(cmd.ClipRect.w - cmd.ClipRect.y);

            if (packet.scissorRect.width && packet.scissorRect.height && (packet.scissorRect.width != framebufferSize.dx || packet.scissorRect.height != framebufferSize.dy))
                packet.options |= rhi::Packet::OPT_OVERRIDE_SCISSOR;
            else
                packet.options &= ~rhi::Packet::OPT_OVERRIDE_SCISSOR;

            rhi::AddPacket(packetList, packet);

            packet.startIndex += cmd.ElemCount;
        }
    }

    rhi::EndPacketList(packetList);
    rhi::EndRenderPass(pass);
}
} // namespace ImGuiImplDetails

namespace ImGui
{
DAVA::float32 Settings::scale = 1.f;
DAVA::float32 Settings::pendingScale = Settings::scale;

void Initialize()
{
    using namespace DAVA;

    DVASSERT(!ImGuiImplDetails::initialized);

    ImGuiIO& io = ImGui::GetIO();
    io.RenderDrawListsFn = ImGuiImplDetails::ImGuiDrawFn;

    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    io.KeyMap[ImGuiKey_Tab] = int32(eInputElements::KB_TAB);
    io.KeyMap[ImGuiKey_LeftArrow] = int32(eInputElements::KB_LEFT);
    io.KeyMap[ImGuiKey_RightArrow] = int32(eInputElements::KB_RIGHT);
    io.KeyMap[ImGuiKey_UpArrow] = int32(eInputElements::KB_UP);
    io.KeyMap[ImGuiKey_DownArrow] = int32(eInputElements::KB_DOWN);
    io.KeyMap[ImGuiKey_PageUp] = int32(eInputElements::KB_PAGEUP);
    io.KeyMap[ImGuiKey_PageDown] = int32(eInputElements::KB_PAGEDOWN);
    io.KeyMap[ImGuiKey_Home] = int32(eInputElements::KB_HOME);
    io.KeyMap[ImGuiKey_End] = int32(eInputElements::KB_END);
    io.KeyMap[ImGuiKey_Delete] = int32(eInputElements::KB_DELETE);
    io.KeyMap[ImGuiKey_Backspace] = int32(eInputElements::KB_BACKSPACE);
    io.KeyMap[ImGuiKey_Enter] = int32(eInputElements::KB_ENTER);
    io.KeyMap[ImGuiKey_Escape] = int32(eInputElements::KB_ESCAPE);
    io.KeyMap[ImGuiKey_A] = int32(eInputElements::KB_A);
    io.KeyMap[ImGuiKey_C] = int32(eInputElements::KB_C);
    io.KeyMap[ImGuiKey_V] = int32(eInputElements::KB_V);
    io.KeyMap[ImGuiKey_X] = int32(eInputElements::KB_X);
    io.KeyMap[ImGuiKey_Y] = int32(eInputElements::KB_Y);
    io.KeyMap[ImGuiKey_Z] = int32(eInputElements::KB_Z);

    //vertex layouts
    rhi::VertexLayout vLayout;
    vLayout.AddElement(rhi::VS_TEXCOORD, 0, rhi::VDT_FLOAT, 2);
    vLayout.AddElement(rhi::VS_TEXCOORD, 1, rhi::VDT_FLOAT, 2);
    vLayout.AddElement(rhi::VS_COLOR, 0, rhi::VDT_UINT8N, 4);
    ImGuiImplDetails::vertexLayout = rhi::VertexLayout::UniqueId(vLayout);

    //font sampler-state
    rhi::SamplerState::Descriptor ss_desc;
    ss_desc.fragmentSamplerCount = 1;
    ss_desc.fragmentSampler[0].minFilter = rhi::TEXFILTER_LINEAR;
    ss_desc.fragmentSampler[0].magFilter = rhi::TEXFILTER_LINEAR;
    ss_desc.fragmentSampler[0].mipFilter = rhi::TEXMIPFILTER_NONE;
    ImGuiImplDetails::fontSamplerState = rhi::AcquireSamplerState(ss_desc);

    //depth state
    rhi::DepthStencilState::Descriptor ds_desc;
    ds_desc.depthTestEnabled = false;
    ds_desc.depthWriteEnabled = false;
    ImGuiImplDetails::depthState = rhi::AcquireDepthStencilState(ds_desc);

    //pc pipeline state
    rhi::ShaderSource vp_pc;
    rhi::ShaderSource fp_pc;

    if (vp_pc.Construct(rhi::PROG_VERTEX, ImGuiImplDetails::vprogPC) && fp_pc.Construct(rhi::PROG_FRAGMENT, ImGuiImplDetails::fprogPC))
    {
        rhi::PipelineState::Descriptor ps_desc;

        ps_desc.vertexLayout = vp_pc.ShaderVertexLayout();
        ps_desc.vprogUid = FastName("imgui.vp.pc");
        ps_desc.fprogUid = FastName("imgui.fp.pc");
        ps_desc.blending = fp_pc.Blending();

        const std::string& vp_bin = vp_pc.GetSourceCode(rhi::HostApi());
        const std::string& fp_bin = fp_pc.GetSourceCode(rhi::HostApi());

        rhi::ShaderCache::UpdateProgBinary(rhi::HostApi(), rhi::PROG_VERTEX, ps_desc.vprogUid, vp_bin.c_str(), unsigned(vp_bin.length()));
        rhi::ShaderCache::UpdateProgBinary(rhi::HostApi(), rhi::PROG_FRAGMENT, ps_desc.fprogUid, fp_bin.c_str(), unsigned(fp_bin.length()));

        ImGuiImplDetails::pipelineStatePC = rhi::AcquireRenderPipelineState(ps_desc);
        rhi::CreateVertexConstBuffers(ImGuiImplDetails::pipelineStatePC, 1, &ImGuiImplDetails::constBufferPC);
    }

    //ptc pipeline-state
    rhi::ShaderSource vp_ptc;
    rhi::ShaderSource fp_ptc;

    if (vp_ptc.Construct(rhi::PROG_VERTEX, ImGuiImplDetails::vprogPTC) && fp_ptc.Construct(rhi::PROG_FRAGMENT, ImGuiImplDetails::fprogPTC))
    {
        rhi::PipelineState::Descriptor ps_desc;

        ps_desc.vertexLayout = vp_ptc.ShaderVertexLayout();
        ps_desc.vprogUid = FastName("imgui.vp.ptc");
        ps_desc.fprogUid = FastName("imgui.fp.ptc");
        ps_desc.blending = fp_ptc.Blending();

        const std::string& vp_bin = vp_ptc.GetSourceCode(rhi::HostApi());
        const std::string& fp_bin = fp_ptc.GetSourceCode(rhi::HostApi());

        rhi::ShaderCache::UpdateProgBinary(rhi::HostApi(), rhi::PROG_VERTEX, ps_desc.vprogUid, vp_bin.c_str(), unsigned(vp_bin.length()));
        rhi::ShaderCache::UpdateProgBinary(rhi::HostApi(), rhi::PROG_FRAGMENT, ps_desc.fprogUid, fp_bin.c_str(), unsigned(fp_bin.length()));

        ImGuiImplDetails::pipelineStatePTC = rhi::AcquireRenderPipelineState(ps_desc);
        rhi::CreateVertexConstBuffers(ImGuiImplDetails::pipelineStatePTC, 1, &ImGuiImplDetails::constBufferPTC);
    }

    ImGuiImplDetails::trackedObject = new TrackedObject();
    Engine::Instance()->beginFrame.Connect(ImGuiImplDetails::trackedObject, &OnFrameBegin);
    Engine::Instance()->endFrame.Connect(ImGuiImplDetails::trackedObject, &OnFrameEnd);

    ImGuiImplDetails::inputHandlerToken = InputSystem::Instance()->AddHandler
                                          (eInputDevices::TOUCH_SURFACE | eInputDevices::MOUSE | eInputDevices::KEYBOARD,
                                           MakeFunction<bool, const InputEvent&>(&OnInput));

    ImGuiImplDetails::initialized = true;
}

bool IsInitialized()
{
    return ImGuiImplDetails::initialized;
}

void OnFrameBegin()
{
    using namespace DAVA;

    if (!ImGuiImplDetails::initialized)
        return;

    ImGui::GetIO().DeltaTime = SystemTimer::GetFrameDelta();

    ImGuiImplDetails::framebufferSize.dx = int32(Renderer::GetFramebufferWidth());
    ImGuiImplDetails::framebufferSize.dy = int32(Renderer::GetFramebufferHeight());

    ImGui::GetIO().DisplaySize.x = static_cast<float32>(screenWidth) / Settings::scale;
    ImGui::GetIO().DisplaySize.y = static_cast<float32>(screenHeight) / Settings::scale;

    //check whether to recreate font texture
    for (ImFont* font : ImGui::GetIO().Fonts->Fonts)
    {
        if (!font->IsLoaded())
        {
            if (ImGuiImplDetails::fontTexture.IsValid())
                rhi::DeleteTexture(ImGuiImplDetails::fontTexture);

            if (ImGuiImplDetails::fontTextureSet)
                rhi::ReleaseTextureSet(ImGuiImplDetails::fontTextureSet);

            ImGuiImplDetails::fontTexture = rhi::HTexture();
            ImGuiImplDetails::fontTextureSet = rhi::HTextureSet();

            break;
        }
    }

    //create font texture if needed
    if (!ImGuiImplDetails::fontTexture.IsValid())
    {
        ImGuiIO& io = ImGui::GetIO();
        uint8* pixels;
        int32 width, height, bytes_per_pixel;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);

        rhi::Texture::Descriptor tex_desc;
        tex_desc.width = uint32(width);
        tex_desc.height = uint32(height);
        tex_desc.format = rhi::TextureFormat::TEXTURE_FORMAT_R8G8B8A8;
        tex_desc.initialData[0] = pixels;
        ImGuiImplDetails::fontTexture = rhi::CreateTexture(tex_desc);

        rhi::TextureSetDescriptor set_desc;
        set_desc.fragmentTextureCount = 1;
        set_desc.fragmentTexture[0] = ImGuiImplDetails::fontTexture;
        ImGuiImplDetails::fontTextureSet = rhi::AcquireTextureSet(set_desc);

        io.Fonts->SetTexID(&ImGuiImplDetails::fontTextureSet);
        io.Fonts->ClearTexData();
    }

    //check if need restore font texture after reset
    if (ImGuiImplDetails::fontTexture.IsValid() && rhi::NeedRestoreTexture(ImGuiImplDetails::fontTexture))
    {
        ImGuiIO& io = ImGui::GetIO();

        uint8* pixels;
        int32 width, height, bytes_per_pixel;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);
        rhi::UpdateTexture(ImGuiImplDetails::fontTexture, pixels, 0);
        io.Fonts->SetTexID(&ImGuiImplDetails::fontTextureSet);
        io.Fonts->ClearTexData();
    }

    ImGui::NewFrame();
}

void OnFrameEnd()
{
    if (ImGuiImplDetails::initialized)
    {
        ImGui::Render();

        DAVA::float32 x = Settings::scale / Settings::pendingScale;

        ImGuiContext* ctx = ImGui::GetCurrentContext();

        for (ImGuiWindow* window : ctx->Windows)
        {
            window->PosFloat.x *= x;
            window->PosFloat.y *= x;
        }

        Settings::scale = Settings::pendingScale;
    }
}

bool OnInput(const DAVA::InputEvent& input)
{
    using namespace DAVA;

    if (!ImGuiImplDetails::initialized)
    {
        return false;
    }

    auto Convert = [](const AnalogElementState& pos) {
        Vector2 coords = ImGuiUtils::ConvertInputCoordsToImGuiCoords({ pos.x, pos.y }) / Settings::scale;
        return ImVec2{ coords.x, coords.y };
    };

    bool pointerInputToProcess = false;

    ImGuiIO& io = ImGui::GetIO();

    if (input.deviceType == eInputDevices::MOUSE)
    {
        Mouse* mouse = GetEngineContext()->deviceManager->GetMouse();

        io.MousePos = Convert(mouse->GetPosition());
        pointerInputToProcess = true;

        if (IsMouseButtonInputElement(input.elementId))
        {
            uint32 mouseButtonIndex = input.elementId - MOUSE_FIRST_BUTTON;
            io.MouseDown[mouseButtonIndex] = input.digitalState.IsPressed();
        }
        else if (input.elementId == MOUSE_WHEEL)
        {
            AnalogElementState mouseWheelDelta = mouse->GetWheelDelta();
            io.MouseWheel = mouseWheelDelta.y;
        }
    }
    else if (input.deviceType == eInputDevices::KEYBOARD)
    {
        if (input.keyboardEvent.charCode > 0)
        {
            io.AddInputCharacter(ImWchar(input.keyboardEvent.charCode));
        }
        else
        {
            bool keyIsPressed = input.digitalState.IsPressed();

            io.KeysDown[static_cast<size_t>(input.elementId) - KB_FIRST] = keyIsPressed;

            if (IsKeyboardModifierInputElement(input.elementId))
            {
                if (input.elementId == KB_LCTRL || input.elementId == KB_RCTRL)
                {
                    io.KeyCtrl = keyIsPressed;
                }
                else if (input.elementId == KB_LALT || input.elementId == KB_RALT)
                {
                    io.KeyAlt = keyIsPressed;
                }
                else if (input.elementId == KB_LCMD || input.elementId == KB_RCMD)
                {
                    io.KeySuper = keyIsPressed;
                }
            }
        }
    }
    else if (input.deviceType == eInputDevices::TOUCH_SURFACE)
    {
        if (input.elementId == TOUCH_POSITION0 || input.elementId == TOUCH_CLICK0)
        {
            TouchScreen* touchScreen = static_cast<TouchScreen*>(input.device);

            io.MousePos = Convert(touchScreen->GetTouchPositionByIndex(0));

            pointerInputToProcess = true;

            if (input.elementId == TOUCH_CLICK0)
            {
                if (input.digitalState.IsJustPressed())
                {
                    io.MouseDown[0] = true;
                }
                else if (input.digitalState.IsJustReleased())
                {
                    io.MouseDown[0] = false;
                }
            }
        }
    }

    // This is just an ugly hack, but it works on current frame unlike io.Want* functions
    if (pointerInputToProcess)
    {
        return ImGui::IsPosHoveringAnyWindow(io.MousePos);
    }

    return io.WantCaptureKeyboard || io.WantTextInput;
}

void Uninitialize()
{
    if (ImGuiImplDetails::initialized)
    {
        for (ImGuiImplDetails::DynamicBuffers* buffers : ImGuiImplDetails::dynamicBuffersIdle)
        {
            DVASSERT(buffers != nullptr);
            DeleteDynamicBuffers(buffers);
        }
        ImGuiImplDetails::dynamicBuffersIdle.clear();

        for (ImGuiImplDetails::DynamicBuffers* buffers : ImGuiImplDetails::dynamicBuffersUsed)
        {
            DVASSERT(buffers != nullptr);
            DeleteDynamicBuffers(buffers);
        }
        ImGuiImplDetails::dynamicBuffersUsed.clear();

        rhi::DeleteTexture(ImGuiImplDetails::fontTexture);
        rhi::ReleaseTextureSet(ImGuiImplDetails::fontTextureSet);
        rhi::ReleaseRenderPipelineState(ImGuiImplDetails::pipelineStatePC);
        rhi::ReleaseRenderPipelineState(ImGuiImplDetails::pipelineStatePTC);
        rhi::ReleaseDepthStencilState(ImGuiImplDetails::depthState);
        rhi::DeleteConstBuffer(ImGuiImplDetails::constBufferPC);
        rhi::DeleteConstBuffer(ImGuiImplDetails::constBufferPTC);

        ImGui::Shutdown();

        DAVA::InputSystem::Instance()->RemoveHandler(ImGuiImplDetails::inputHandlerToken);
        ImGuiImplDetails::inputHandlerToken = 0;

        DAVA::SafeDelete(ImGuiImplDetails::trackedObject);

        ImGuiImplDetails::initialized = false;
    }
}
} // namespace ImGui