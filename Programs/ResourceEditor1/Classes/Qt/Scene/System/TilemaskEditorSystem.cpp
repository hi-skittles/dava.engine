#include "TilemaskEditorSystem.h"
#include "CollisionSystem.h"
#include "ModifSystem.h"
#include "LandscapeEditorDrawSystem.h"
#include "Classes/Qt/Scene/SceneEditor2.h"
#include "LandscapeEditorDrawSystem/HeightmapProxy.h"
#include "LandscapeEditorDrawSystem/LandscapeProxy.h"
#include "Commands2/TilemaskEditorCommands.h"
#include "Qt/Main/QtUtils.h"

#include "Classes/Selection/Selection.h"
#include "Classes/Qt/Main/QtUtils.h"

#include <UI/UIEvent.h>
#include <Render/Renderer.h>
#include <Render/Image/ImageConvert.h>
#include <Render/DynamicBufferAllocator.h>

#include <QApplication>

static std::array<DAVA::FastName, 4> TILECOLOR_PARAM_NAMES;

static const DAVA::FastName TILEMASK_EDITOR_FLAG_DRAW_TYPE("DRAW_TYPE");
static const DAVA::FastName TILEMASK_EDITOR_PARAM_INTENSITY("intensity");
static const DAVA::FastName TILEMASK_EDITOR_PARAM_COPYPASTE_OFFSET("copypasteOffset");

static const DAVA::FastName TILEMASK_EDTIOR_TEXTURE_SOURCE("sourceTexture");
static const DAVA::FastName TILEMASK_EDTIOR_TEXTURE_TOOL("toolTexture");

static const DAVA::FastName TILEMASK_EDITOR_MATERIAL_PASS("2d");

TilemaskEditorSystem::TilemaskEditorSystem(DAVA::Scene* scene)
    : LandscapeEditorSystem(scene, DefaultCursorPath())
    , curToolSize(0)
    , toolImageTexture(nullptr)
    , landscapeTilemaskTexture(nullptr)
    , tileTextureNum(0)
    , drawingType(TILEMASK_DRAW_NORMAL)
    , strength(0.25f)
    , toolImagePath("")
    , toolImageIndex(0)
    , copyPasteFrom(-1.f, -1.f)
    , editingIsEnabled(false)
    , toolTexture(NULL)
    , toolSpriteUpdated(false)
    , needCreateUndo(false)
    , textureLevel(DAVA::Landscape::TEXTURE_TILEMASK)
{
    curToolSize = 120;

    editorMaterial = new DAVA::NMaterial();
    editorMaterial->SetFXName(DAVA::FastName("~res:/Materials/Landscape.Tilemask.Editor.material"));
    editorMaterial->AddFlag(TILEMASK_EDITOR_FLAG_DRAW_TYPE, 0);
    editorMaterial->AddProperty(TILEMASK_EDITOR_PARAM_INTENSITY, &strength, rhi::ShaderProp::TYPE_FLOAT1);
    editorMaterial->AddProperty(TILEMASK_EDITOR_PARAM_COPYPASTE_OFFSET, copyPasteOffset.data, rhi::ShaderProp::TYPE_FLOAT2);

    editorMaterial->PreBuildMaterial(TILEMASK_EDITOR_MATERIAL_PASS);

    quadPacket.vertexStreamCount = 1;
    quadPacket.vertexCount = 6;
    quadPacket.primitiveType = rhi::PRIMITIVE_TRIANGLELIST;
    quadPacket.primitiveCount = 2;

    rhi::VertexLayout layout;
    layout.AddElement(rhi::VS_POSITION, 0, rhi::VDT_FLOAT, 3);
    layout.AddElement(rhi::VS_TEXCOORD, 0, rhi::VDT_FLOAT, 2);

    quadPacket.vertexLayoutUID = rhi::VertexLayout::UniqueId(layout);

    TILECOLOR_PARAM_NAMES[0] = DAVA::Landscape::PARAM_TILE_COLOR0;
    TILECOLOR_PARAM_NAMES[1] = DAVA::Landscape::PARAM_TILE_COLOR1;
    TILECOLOR_PARAM_NAMES[2] = DAVA::Landscape::PARAM_TILE_COLOR2;
    TILECOLOR_PARAM_NAMES[3] = DAVA::Landscape::PARAM_TILE_COLOR3;
}

TilemaskEditorSystem::~TilemaskEditorSystem()
{
    SafeRelease(editorMaterial);

    SafeRelease(toolImageTexture);
    SafeRelease(toolTexture);
}

LandscapeEditorDrawSystem::eErrorType TilemaskEditorSystem::EnableLandscapeEditing()
{
    if (enabled)
    {
        return LandscapeEditorDrawSystem::LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS;
    }

    LandscapeEditorDrawSystem::eErrorType canBeEnabledError = IsCanBeEnabled();
    if (canBeEnabledError != LandscapeEditorDrawSystem::LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS)
    {
        return canBeEnabledError;
    }

    LandscapeEditorDrawSystem::eErrorType enablingError = drawSystem->EnableTilemaskEditing();
    if (enablingError != LandscapeEditorDrawSystem::LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS)
    {
        return enablingError;
    }

    drawSystem->UpdateTilemaskPathname();
    bool inited = drawSystem->InitTilemaskImageCopy();
    if (!inited)
    {
        return LandscapeEditorDrawSystem::LANDSCAPE_EDITOR_SYSTEM_TILEMASK_TEXTURE_ABSENT;
    }

    bool inputLocked = AcquireInputLock(GetScene());
    DVASSERT(inputLocked);
    Selection::Lock();

    landscapeSize = drawSystem->GetTextureSize(textureLevel);
    copyPasteFrom = DAVA::Vector2(-1.f, -1.f);

    drawSystem->EnableCursor();
    drawSystem->EnableCustomDraw();
    drawSystem->SetCursorTexture(cursorTexture);
    drawSystem->SetCursorSize(cursorSize);
    SetBrushSize(curToolSize);

    InitSprites();

    DAVA::Texture* srcSprite = drawSystem->GetLandscapeProxy()->GetTilemaskDrawTexture(LandscapeProxy::TILEMASK_TEXTURE_SOURCE);
    DAVA::Texture* dstSprite = drawSystem->GetLandscapeProxy()->GetTilemaskDrawTexture(LandscapeProxy::TILEMASK_TEXTURE_DESTINATION);

    srcSprite->SetMinMagFilter(rhi::TEXFILTER_LINEAR, rhi::TEXFILTER_LINEAR, rhi::TEXMIPFILTER_NONE);
    dstSprite->SetMinMagFilter(rhi::TEXFILTER_LINEAR, rhi::TEXFILTER_LINEAR, rhi::TEXMIPFILTER_NONE);

    editorMaterial->AddTexture(TILEMASK_EDTIOR_TEXTURE_TOOL, toolTexture);
    editorMaterial->AddTexture(TILEMASK_EDTIOR_TEXTURE_SOURCE, srcSprite);

    enabled = true;
    return LandscapeEditorDrawSystem::LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS;
}

bool TilemaskEditorSystem::DisableLandscapeEdititing()
{
    if (!enabled)
    {
        return true;
    }

    FinishEditing();
    needCreateUndo = false;

    ReleaseInputLock(GetScene());
    Selection::Unlock();

    drawSystem->DisableCursor();
    drawSystem->DisableCustomDraw();
    drawSystem->DisableTilemaskEditing();

    editorMaterial->RemoveTexture(TILEMASK_EDTIOR_TEXTURE_TOOL);
    editorMaterial->RemoveTexture(TILEMASK_EDTIOR_TEXTURE_SOURCE);

    SafeRelease(landscapeTilemaskTexture);

    enabled = false;
    return !enabled;
}

void TilemaskEditorSystem::Process(DAVA::float32 timeElapsed)
{
    if (!IsLandscapeEditingEnabled())
    {
        return;
    }

    if (editingIsEnabled && isIntersectsLandscape)
    {
        if (prevCursorPos != cursorPosition)
        {
            prevCursorPos = cursorPosition;

            DAVA::Vector2 toolSize = DAVA::Vector2(static_cast<DAVA::float32>(curToolSize), static_cast<DAVA::float32>(curToolSize));
            DAVA::Vector2 toolPos = cursorPosition * landscapeSize - toolSize / 2.f;
            DAVA::Rect toolRect(std::floor(toolPos.x), std::floor(toolPos.y), std::ceil(toolSize.x), std::ceil(toolSize.y));

            DAVA::RenderSystem2D::RenderTargetPassDescriptor desc;
            desc.priority = DAVA::PRIORITY_SERVICE_2D;
            desc.colorAttachment = toolTexture->handle;
            desc.depthAttachment = toolTexture->handleDepthStencil;
            desc.width = toolTexture->GetWidth();
            desc.height = toolTexture->GetHeight();
            desc.transformVirtualToPhysical = false;
            DAVA::RenderSystem2D::Instance()->BeginRenderTargetPass(desc);
            DAVA::RenderSystem2D::Instance()->DrawTexture(toolImageTexture, DAVA::RenderSystem2D::DEFAULT_2D_TEXTURE_MATERIAL, DAVA::Color::White, toolRect);
            DAVA::RenderSystem2D::Instance()->EndRenderTargetPass();

            toolSpriteUpdated = true;

            if (activeDrawingType == TILEMASK_DRAW_COPY_PASTE)
            {
                editorMaterial->SetPropertyValue(TILEMASK_EDITOR_PARAM_COPYPASTE_OFFSET, copyPasteOffset.data);
            }

            AddRectToAccumulator(toolRect);
        }
    }
}

bool TilemaskEditorSystem::Input(DAVA::UIEvent* event)
{
    if (!IsLandscapeEditingEnabled())
    {
        return false;
    }

    UpdateCursorPosition();

    if (event->mouseButton == DAVA::eMouseButtons::LEFT)
    {
        DAVA::Vector3 point;

        switch (event->phase)
        {
        case DAVA::UIEvent::Phase::BEGAN:
            if (isIntersectsLandscape && !needCreateUndo)
            {
                if (drawingType == TILEMASK_DRAW_COPY_PASTE)
                {
                    DAVA::int32 curKeyModifiers = QApplication::keyboardModifiers();
                    if (curKeyModifiers & Qt::AltModifier)
                    {
                        copyPasteFrom = cursorPosition;
                        copyPasteOffset = DAVA::Vector2();
                        return false;
                    }
                    else
                    {
                        if (copyPasteFrom == DAVA::Vector2(-1.f, -1.f))
                        {
                            return false;
                        }
                        copyPasteOffset = copyPasteFrom - cursorPosition;
                    }
                }

                ResetAccumulatorRect();
                editingIsEnabled = true;
                activeDrawingType = drawingType;
            }
            break;

        case DAVA::UIEvent::Phase::DRAG:
            break;

        case DAVA::UIEvent::Phase::ENDED:
            FinishEditing();
            break;

        default:
            break;
        }
    }
    return false;
}

void TilemaskEditorSystem::InputCancelled(DAVA::UIEvent* event)
{
    if (IsLandscapeEditingEnabled() && (event->mouseButton == DAVA::eMouseButtons::LEFT))
    {
        FinishEditing();
    }
}

void TilemaskEditorSystem::FinishEditing()
{
    if (editingIsEnabled)
    {
        needCreateUndo = true;
        editingIsEnabled = false;
    }
    prevCursorPos = DAVA::Vector2(-1.f, -1.f);
}

void TilemaskEditorSystem::SetBrushSize(DAVA::int32 brushSize)
{
    if (brushSize > 0)
    {
        curToolSize = brushSize;
        cursorSize = static_cast<DAVA::float32>(brushSize) / landscapeSize;
        drawSystem->SetCursorSize(cursorSize);

        UpdateToolImage();
    }
}

void TilemaskEditorSystem::SetStrength(DAVA::float32 strength_)
{
    if (strength_ >= 0)
    {
        strength = strength_;
        editorMaterial->SetPropertyValue(TILEMASK_EDITOR_PARAM_INTENSITY, &strength);
    }
}

void TilemaskEditorSystem::SetToolImage(const DAVA::FilePath& toolImagePath, DAVA::int32 index)
{
    this->toolImagePath = toolImagePath;
    this->toolImageIndex = index;
    UpdateToolImage();
}

void TilemaskEditorSystem::SetTileTexture(DAVA::uint32 tileTexture)
{
    if (tileTexture >= GetTileTextureCount())
    {
        return;
    }

    tileTextureNum = tileTexture;

    editorMaterial->SetFlag(TILEMASK_EDITOR_FLAG_DRAW_TYPE, tileTextureNum);
    editorMaterial->PreBuildMaterial(TILEMASK_EDITOR_MATERIAL_PASS);
}

void TilemaskEditorSystem::UpdateBrushTool()
{
    struct QuadVertex
    {
        DAVA::Vector3 position;
        DAVA::Vector2 texCoord;
    };

    if (drawingType == TILEMASK_DRAW_COPY_PASTE && (copyPasteFrom == DAVA::Vector2(-1.f, -1.f)))
        return;

    DAVA::Texture* srcTexture = drawSystem->GetLandscapeProxy()->GetTilemaskDrawTexture(LandscapeProxy::TILEMASK_TEXTURE_SOURCE);
    DAVA::Texture* dstTexture = drawSystem->GetLandscapeProxy()->GetTilemaskDrawTexture(LandscapeProxy::TILEMASK_TEXTURE_DESTINATION);

    editorMaterial->SetTexture(TILEMASK_EDTIOR_TEXTURE_SOURCE, srcTexture);

    rhi::RenderPassConfig passConf;
    passConf.colorBuffer[0].texture = dstTexture->handle;
    passConf.priority = DAVA::PRIORITY_SERVICE_2D;
    passConf.viewport.width = dstTexture->GetWidth();
    passConf.viewport.height = dstTexture->GetHeight();
    passConf.colorBuffer[0].loadAction = rhi::LOADACTION_CLEAR;
    Memset(passConf.colorBuffer[0].clearColor, 0, 4 * sizeof(DAVA::float32));

    editorMaterial->PreBuildMaterial(TILEMASK_EDITOR_MATERIAL_PASS);
    editorMaterial->BindParams(quadPacket);

    DAVA::DynamicBufferAllocator::AllocResultVB quadBuffer = DAVA::DynamicBufferAllocator::AllocateVertexBuffer(sizeof(QuadVertex), 4);
    QuadVertex* quadVertices = reinterpret_cast<QuadVertex*>(quadBuffer.data);

    quadVertices[0].position = DAVA::Vector3(-1.f, -1.f, .0f);
    quadVertices[1].position = DAVA::Vector3(-1.f, 1.f, .0f);
    quadVertices[2].position = DAVA::Vector3(1.f, -1.f, .0f);
    quadVertices[3].position = DAVA::Vector3(1.f, 1.f, .0f);

    if (rhi::DeviceCaps().isCenterPixelMapping)
    {
        const DAVA::float32 pixelOffset = 1.f / srcTexture->GetWidth();
        for (DAVA::uint32 i = 0; i < 4; ++i)
        {
            quadVertices[i].position.x -= pixelOffset;
            quadVertices[i].position.y -= pixelOffset;
        }
    }

    if (rhi::DeviceCaps().isUpperLeftRTOrigin)
    {
        for (DAVA::uint32 i = 0; i < 4; ++i)
            quadVertices[i].position.y = -quadVertices[i].position.y;
    }

    quadVertices[0].texCoord = DAVA::Vector2(0.f, 0.f);
    quadVertices[1].texCoord = DAVA::Vector2(0.f, 1.f);
    quadVertices[2].texCoord = DAVA::Vector2(1.f, 0.f);
    quadVertices[3].texCoord = DAVA::Vector2(1.f, 1.f);

    quadPacket.vertexStream[0] = quadBuffer.buffer;
    quadPacket.baseVertex = quadBuffer.baseVertex;
    quadPacket.indexBuffer = DAVA::DynamicBufferAllocator::AllocateQuadListIndexBuffer(1);

    rhi::HPacketList pList;
    rhi::HRenderPass pass = rhi::AllocateRenderPass(passConf, 1, &pList);

    rhi::BeginRenderPass(pass);
    rhi::BeginPacketList(pList);

    rhi::AddPacket(pList, quadPacket);

    rhi::EndPacketList(pList);
    rhi::EndRenderPass(pass);

    drawSystem->SetTileMaskTexture(dstTexture);
    drawSystem->GetLandscapeProxy()->SwapTilemaskDrawTextures();
}

void TilemaskEditorSystem::UpdateToolImage()
{
    SafeRelease(toolImageTexture);

    DAVA::Vector<DAVA::Image*> images;
    DAVA::ImageSystem::Load(toolImagePath, images);
    if (images.size())
    {
        DVASSERT(images.size() == 1);
        DVASSERT(images[0]->GetPixelFormat() == DAVA::FORMAT_RGBA8888);

        DAVA::ScopedPtr<DAVA::Image> toolImage(DAVA::Image::Create(curToolSize, curToolSize, DAVA::FORMAT_RGBA8888));
        DAVA::ImageConvert::ResizeRGBA8Billinear(reinterpret_cast<DAVA::uint32*>(images[0]->data), images[0]->GetWidth(), images[0]->GetHeight(),
                                                 reinterpret_cast<DAVA::uint32*>(toolImage->data), curToolSize, curToolSize);

        SafeRelease(images[0]);

        toolImageTexture = DAVA::Texture::CreateFromData(toolImage, false);
    }
}

void TilemaskEditorSystem::AddRectToAccumulator(const DAVA::Rect& rect)
{
    updatedRectAccumulator = updatedRectAccumulator.Combine(rect);
}

void TilemaskEditorSystem::ResetAccumulatorRect()
{
    DAVA::float32 inf = std::numeric_limits<DAVA::float32>::infinity();
    updatedRectAccumulator = DAVA::Rect(inf, inf, -inf, -inf);
}

DAVA::Rect TilemaskEditorSystem::GetUpdatedRect()
{
    DAVA::Rect r = updatedRectAccumulator;
    drawSystem->ClampToTexture(textureLevel, r);

    return r;
}

DAVA::uint32 TilemaskEditorSystem::GetTileTextureCount() const
{
    return 4;
}

DAVA::Texture* TilemaskEditorSystem::GetTileTexture()
{
    return drawSystem->GetLandscapeProxy()->GetLandscapeTexture(DAVA::Landscape::TEXTURE_TILE);
}

DAVA::Color TilemaskEditorSystem::GetTileColor(DAVA::uint32 index)
{
    if (index < GetTileTextureCount())
    {
        return drawSystem->GetLandscapeProxy()->GetLandscapeTileColor(TILECOLOR_PARAM_NAMES[index]);
    }

    return DAVA::Color::Black;
}

void TilemaskEditorSystem::SetTileColor(DAVA::int32 index, const DAVA::Color& color)
{
    if (index < 0 || index >= static_cast<DAVA::int32>(GetTileTextureCount()))
    {
        return;
    }

    DAVA::Color curColor = drawSystem->GetLandscapeProxy()->GetLandscapeTileColor(TILECOLOR_PARAM_NAMES[index]);

    if (curColor != color)
    {
        SceneEditor2* scene = (SceneEditor2*)(GetScene());
        scene->Exec(std::unique_ptr<DAVA::Command>(new SetTileColorCommand(drawSystem->GetLandscapeProxy(), TILECOLOR_PARAM_NAMES[index], color)));
    }
}

void TilemaskEditorSystem::CreateMaskTexture()
{
    DAVA::Texture* tilemask = drawSystem->GetLandscapeProxy()->GetLandscapeTexture(DAVA::Landscape::TEXTURE_TILEMASK);
    DAVA::Texture* srcTexture = drawSystem->GetLandscapeProxy()->GetTilemaskDrawTexture(LandscapeProxy::TILEMASK_TEXTURE_SOURCE);

    if (tilemask != srcTexture)
    {
        landscapeTilemaskTexture = SafeRetain(tilemask);

        DAVA::Rect destRect(0.0f, 0.0f, landscapeTilemaskTexture->width, landscapeTilemaskTexture->height);
        DAVA::Rect sourceRect(0.0f, 0.0f, 1.0f, 1.0f);

        DAVA::RenderSystem2D::RenderTargetPassDescriptor desc;
        desc.priority = DAVA::PRIORITY_SERVICE_2D;
        desc.colorAttachment = srcTexture->handle;
        desc.depthAttachment = srcTexture->handleDepthStencil;
        desc.width = srcTexture->GetWidth();
        desc.height = srcTexture->GetHeight();
        desc.transformVirtualToPhysical = false;
        desc.clearTarget = true;

        DAVA::RenderSystem2D::Instance()->BeginRenderTargetPass(desc);
        DAVA::RenderSystem2D::Instance()->DrawTextureWithoutAdjustingRects(landscapeTilemaskTexture, DAVA::RenderSystem2D::DEFAULT_2D_TEXTURE_NOBLEND_MATERIAL,
                                                                           DAVA::Color::White, destRect, sourceRect);
        DAVA::RenderSystem2D::Instance()->EndRenderTargetPass();

        drawSystem->SetTileMaskTexture(srcTexture);
    }
}

void TilemaskEditorSystem::Draw()
{
    if (!IsLandscapeEditingEnabled())
    {
        return;
    }

    if (toolSpriteUpdated)
    {
        UpdateBrushTool();
        toolSpriteUpdated = false;
    }

    if (needCreateUndo)
    {
        CreateUndoPoint();
        needCreateUndo = false;
    }
}

void TilemaskEditorSystem::CreateUndoPoint()
{
    DAVA::Rect rect = GetUpdatedRect();
    if (rect.dx > 0 && rect.dy > 0)
    {
        SceneEditor2* scene = dynamic_cast<SceneEditor2*>(GetScene());
        DVASSERT(scene);
        scene->Exec(std::unique_ptr<DAVA::Command>(new ModifyTilemaskCommand(drawSystem->GetLandscapeProxy(), rect)));
    }
}

DAVA::int32 TilemaskEditorSystem::GetBrushSize()
{
    return curToolSize;
}

DAVA::float32 TilemaskEditorSystem::GetStrength()
{
    return strength;
}

DAVA::int32 TilemaskEditorSystem::GetToolImage()
{
    return toolImageIndex;
}

DAVA::uint32 TilemaskEditorSystem::GetTileTextureIndex()
{
    return tileTextureNum;
}

void TilemaskEditorSystem::InitSprites()
{
    DAVA::int32 texSize = static_cast<DAVA::int32>(drawSystem->GetTextureSize(textureLevel));
    if (toolTexture != nullptr && texSize != toolTexture->GetWidth())
    {
        DAVA::SafeRelease(toolTexture);
    }

    if (toolTexture == nullptr)
    {
        toolTexture = DAVA::Texture::CreateFBO(texSize, texSize, DAVA::FORMAT_RGBA8888 /*, Texture::DEPTH_NONE*/);
    }

    drawSystem->GetLandscapeProxy()->InitTilemaskDrawTextures();
    CreateMaskTexture();
}

void TilemaskEditorSystem::SetDrawingType(eTilemaskDrawType type)
{
    if (type >= TILEMASK_DRAW_NORMAL && type < TILEMASK_DRAW_TYPES_COUNT)
    {
        drawingType = type;

        if (type == TILEMASK_DRAW_COPY_PASTE)
        {
            editorMaterial->SetFlag(TILEMASK_EDITOR_FLAG_DRAW_TYPE, 4);
        }
        else
        {
            editorMaterial->SetFlag(TILEMASK_EDITOR_FLAG_DRAW_TYPE, tileTextureNum);
        }

        editorMaterial->PreBuildMaterial(TILEMASK_EDITOR_MATERIAL_PASS);
    }
}

TilemaskEditorSystem::eTilemaskDrawType TilemaskEditorSystem::GetDrawingType()
{
    return drawingType;
}
