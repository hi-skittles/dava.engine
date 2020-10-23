#include "../SceneEditor2.h"
#include "LandscapeEditorDrawSystem.h"
#include "LandscapeEditorDrawSystem/LandscapeProxy.h"
#include "LandscapeEditorDrawSystem/HeightmapProxy.h"
#include "LandscapeEditorDrawSystem/CustomColorsProxy.h"
#include "LandscapeEditorDrawSystem/NotPassableTerrainProxy.h"
#include "LandscapeEditorDrawSystem/RulerToolProxy.h"

#include "Commands2/InspMemberModifyCommand.h"
#include "Commands2/SetFieldValueCommand.h"
#include "Commands2/InspDynamicModifyCommand.h"
#include "Commands2/Base/RECommandNotificationObject.h"

#include "Scene/SceneHelper.h"

#include <Scene3D/Systems/RenderUpdateSystem.h>
#include <Debug/DVAssert.h>
#include <Base/Any.h>

LandscapeEditorDrawSystem::LandscapeEditorDrawSystem(DAVA::Scene* scene)
    : SceneSystem(scene)
{
}

LandscapeEditorDrawSystem::~LandscapeEditorDrawSystem()
{
    DAVA::SafeRelease(baseLandscape);
    DAVA::SafeRelease(landscapeProxy);
    DAVA::SafeRelease(heightmapProxy);
    DAVA::SafeRelease(customColorsProxy);
    DAVA::SafeRelease(rulerToolProxy);
    DAVA::SafeDelete(notPassableTerrainProxy);
}

LandscapeProxy* LandscapeEditorDrawSystem::GetLandscapeProxy()
{
    return landscapeProxy;
}

HeightmapProxy* LandscapeEditorDrawSystem::GetHeightmapProxy()
{
    return heightmapProxy;
}

CustomColorsProxy* LandscapeEditorDrawSystem::GetCustomColorsProxy()
{
    return customColorsProxy;
}

RulerToolProxy* LandscapeEditorDrawSystem::GetRulerToolProxy()
{
    return rulerToolProxy;
}

LandscapeEditorDrawSystem::eErrorType LandscapeEditorDrawSystem::EnableCustomDraw()
{
    if (customDrawRequestCount != 0)
    {
        ++customDrawRequestCount;
        return LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS;
    }

    eErrorType initError = Init();
    if (initError != LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS)
    {
        return initError;
    }

    landscapeProxy->SetMode(LandscapeProxy::MODE_CUSTOM_LANDSCAPE);
    landscapeProxy->SetHeightmap(heightmapProxy);

    ++customDrawRequestCount;

    return LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS;
}

void LandscapeEditorDrawSystem::DisableCustomDraw()
{
    if (customDrawRequestCount == 0)
    {
        return;
    }

    --customDrawRequestCount;

    if (customDrawRequestCount == 0)
    {
        landscapeProxy->SetMode(LandscapeProxy::MODE_ORIGINAL_LANDSCAPE);
        UpdateBaseLandscapeHeightmap();
    }
}

bool LandscapeEditorDrawSystem::IsNotPassableTerrainEnabled()
{
    if (!notPassableTerrainProxy)
    {
        return false;
    }

    return notPassableTerrainProxy->IsEnabled();
}

LandscapeEditorDrawSystem::eErrorType LandscapeEditorDrawSystem::IsNotPassableTerrainCanBeEnabled()
{
    return VerifyLandscape();
}

LandscapeEditorDrawSystem::eErrorType LandscapeEditorDrawSystem::EnableNotPassableTerrain()
{
    eErrorType canBeEnabledError = IsNotPassableTerrainCanBeEnabled();
    if (canBeEnabledError != LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS)
    {
        return canBeEnabledError;
    }

    if (!notPassableTerrainProxy)
    {
        notPassableTerrainProxy = new NotPassableTerrainProxy(baseLandscape->GetHeightmap()->Size());
    }

    if (notPassableTerrainProxy->IsEnabled())
    {
        return LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS;
    }

    eErrorType enableCustomDrawError = EnableCustomDraw();
    if (enableCustomDrawError != LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS)
    {
        return enableCustomDrawError;
    }

    DAVA::Rect2i updateRect = DAVA::Rect2i(0, 0, GetHeightmapProxy()->Size(), GetHeightmapProxy()->Size());
    notPassableTerrainProxy->SetEnabled(true);
    notPassableTerrainProxy->UpdateTexture(heightmapProxy, landscapeProxy->GetLandscapeBoundingBox(), updateRect);

    landscapeProxy->SetToolTexture(notPassableTerrainProxy->GetTexture(), false);

    return LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS;
}

bool LandscapeEditorDrawSystem::DisableNotPassableTerrain()
{
    if (!notPassableTerrainProxy || !notPassableTerrainProxy->IsEnabled())
    {
        return false;
    }

    notPassableTerrainProxy->SetEnabled(false);
    landscapeProxy->SetToolTexture(nullptr, false);

    DisableCustomDraw();
    return true;
}

void LandscapeEditorDrawSystem::EnableCursor()
{
    landscapeProxy->CursorEnable();
}

void LandscapeEditorDrawSystem::DisableCursor()
{
    landscapeProxy->CursorDisable();
}

void LandscapeEditorDrawSystem::SetCursorTexture(DAVA::Texture* cursorTexture)
{
    landscapeProxy->SetCursorTexture(cursorTexture);
}

void LandscapeEditorDrawSystem::SetCursorSize(DAVA::float32 cursorSize)
{
    if (landscapeProxy)
    {
        landscapeProxy->SetCursorSize(cursorSize);
    }
}

void LandscapeEditorDrawSystem::SetCursorPosition(const DAVA::Vector2& cursorPos)
{
    if (landscapeProxy)
    {
        landscapeProxy->SetCursorPosition(cursorPos);
    }
}

void LandscapeEditorDrawSystem::Process(DAVA::float32 timeElapsed)
{
    if (heightmapProxy && heightmapProxy->IsHeightmapChanged())
    {
        const DAVA::Rect& changedRect = heightmapProxy->GetChangedRect();
        DAVA::Rect2i updateRect(static_cast<DAVA::int32>(changedRect.x), static_cast<DAVA::int32>(changedRect.y),
                                static_cast<DAVA::int32>(changedRect.dx), static_cast<DAVA::int32>(changedRect.dy));

        if (customDrawRequestCount == 0)
        {
            UpdateBaseLandscapeHeightmap();
        }
        else
        {
            if (baseLandscape->IsUpdatable())
            {
                baseLandscape->UpdatePart(updateRect);
            }
            else
            {
                UpdateBaseLandscapeHeightmap();
            }
        }

        if (notPassableTerrainProxy && notPassableTerrainProxy->IsEnabled())
        {
            notPassableTerrainProxy->UpdateTexture(heightmapProxy, landscapeProxy->GetLandscapeBoundingBox(), updateRect);
        }

        heightmapProxy->ResetHeightmapChanged();
    }

    if (customColorsProxy && customColorsProxy->IsTargetChanged())
    {
        customColorsProxy->ResetTargetChanged();
    }
}

void LandscapeEditorDrawSystem::UpdateBaseLandscapeHeightmap()
{
    DAVA::ScopedPtr<DAVA::Heightmap> h(new DAVA::Heightmap());
    heightmapProxy->Clone(h);

    baseLandscape->SetHeightmap(h);

    GetScene()->foliageSystem->SyncFoliageWithLandscape();
}

DAVA::float32 LandscapeEditorDrawSystem::GetTextureSize(const DAVA::FastName& level)
{
    DAVA::float32 size = 0.f;
    DAVA::Texture* texture = baseLandscape->GetMaterial()->GetEffectiveTexture(level);
    if (texture)
    {
        size = static_cast<DAVA::float32>(texture->GetWidth());
    }
    return size;
}

DAVA::Vector3 LandscapeEditorDrawSystem::GetLandscapeSize()
{
    DAVA::AABBox3 transformedBox;
    baseLandscape->GetBoundingBox().GetTransformedBox(*baseLandscape->GetWorldTransformPtr(), transformedBox);

    DAVA::Vector3 landSize = transformedBox.max - transformedBox.min;
    return landSize;
}

DAVA::float32 LandscapeEditorDrawSystem::GetLandscapeMaxHeight()
{
    DAVA::Vector3 landSize = GetLandscapeSize();
    return landSize.z;
}

DAVA::Rect LandscapeEditorDrawSystem::GetTextureRect(const DAVA::FastName& level)
{
    DAVA::float32 textureSize = GetTextureSize(level);
    return DAVA::Rect(DAVA::Vector2(0.f, 0.f), DAVA::Vector2(textureSize, textureSize));
}

DAVA::Rect LandscapeEditorDrawSystem::GetHeightmapRect()
{
    DAVA::float32 heightmapSize = static_cast<DAVA::float32>(GetHeightmapProxy()->Size());
    return DAVA::Rect(DAVA::Vector2(0.f, 0.f), DAVA::Vector2(heightmapSize, heightmapSize));
}

DAVA::Rect LandscapeEditorDrawSystem::GetLandscapeRect()
{
    DAVA::AABBox3 boundingBox = GetLandscapeProxy()->GetLandscapeBoundingBox();
    DAVA::Vector2 landPos(boundingBox.min.x, boundingBox.min.y);
    DAVA::Vector2 landSize((boundingBox.max - boundingBox.min).x,
                           (boundingBox.max - boundingBox.min).y);

    return DAVA::Rect(landPos, landSize);
}

DAVA::float32 LandscapeEditorDrawSystem::GetHeightAtHeightmapPoint(const DAVA::Vector2& point)
{
    DAVA::Heightmap* heightmap = GetHeightmapProxy();

    DAVA::int32 hmSize = heightmap->Size();
    DAVA::int32 x = static_cast<DAVA::int32>(point.x);
    DAVA::int32 y = static_cast<DAVA::int32>(point.y);

    DVASSERT((x >= 0) && (x < hmSize) && (y >= 0) && (y < hmSize),
             "Point must be in heightmap coordinates");

    int nextX = DAVA::Min(x + 1, hmSize - 1);
    int nextY = DAVA::Min(y + 1, hmSize - 1);
    int i00 = x + y * hmSize;
    int i01 = nextX + y * hmSize;
    int i10 = x + nextY * hmSize;
    int i11 = nextX + nextY * hmSize;

    const auto hmData = heightmap->Data();
    float h00 = static_cast<float>(hmData[i00]);
    float h01 = static_cast<float>(hmData[i01]);
    float h10 = static_cast<float>(hmData[i10]);
    float h11 = static_cast<float>(hmData[i11]);

    float dx = point.x - static_cast<float>(x);
    float dy = point.y - static_cast<float>(y);
    float h0 = h00 * (1.0f - dx) + h01 * dx;
    float h1 = h10 * (1.0f - dx) + h11 * dx;
    float h = h0 * (1.0f - dy) + h1 * dy;

    return h * GetLandscapeMaxHeight() / static_cast<DAVA::float32>(DAVA::Heightmap::MAX_VALUE);
}

DAVA::float32 LandscapeEditorDrawSystem::GetHeightAtTexturePoint(const DAVA::FastName& level, const DAVA::Vector2& point)
{
    auto textureRect = GetTextureRect(level);
    if (textureRect.PointInside(point))
    {
        return GetHeightAtHeightmapPoint(TexturePointToHeightmapPoint(level, point));
    }

    return 0.0f;
}

DAVA::Vector2 LandscapeEditorDrawSystem::HeightmapPointToTexturePoint(const DAVA::FastName& level, const DAVA::Vector2& point)
{
    return TranslatePoint(point, GetHeightmapRect(), GetTextureRect(level));
}

DAVA::Vector2 LandscapeEditorDrawSystem::TexturePointToHeightmapPoint(const DAVA::FastName& level, const DAVA::Vector2& point)
{
    return TranslatePoint(point, GetTextureRect(level), GetHeightmapRect());
}

DAVA::Vector2 LandscapeEditorDrawSystem::TexturePointToLandscapePoint(const DAVA::FastName& level, const DAVA::Vector2& point)
{
    return TranslatePoint(point, GetTextureRect(level), GetLandscapeRect());
}

DAVA::Vector2 LandscapeEditorDrawSystem::LandscapePointToTexturePoint(const DAVA::FastName& level, const DAVA::Vector2& point)
{
    return TranslatePoint(point, GetLandscapeRect(), GetTextureRect(level));
}

DAVA::Vector2 LandscapeEditorDrawSystem::TranslatePoint(const DAVA::Vector2& point, const DAVA::Rect& fromRect, const DAVA::Rect& toRect)
{
    DVASSERT(fromRect.dx != 0.f && fromRect.dy != 0.f);

    DAVA::Vector2 origRectSize = fromRect.GetSize();
    DAVA::Vector2 destRectSize = toRect.GetSize();

    DAVA::Vector2 scale(destRectSize.x / origRectSize.x,
                        destRectSize.y / origRectSize.y);

    DAVA::Vector2 relPos = point - fromRect.GetPosition();
    DAVA::Vector2 newRelPos(relPos.x * scale.x,
                            toRect.dy - 1.0f - relPos.y * scale.y);

    DAVA::Vector2 newPos = newRelPos + toRect.GetPosition();

    return newPos;
}

DAVA::KeyedArchive* LandscapeEditorDrawSystem::GetLandscapeCustomProperties()
{
    return GetOrCreateCustomProperties(landscapeNode)->GetArchive();
}

LandscapeEditorDrawSystem::eErrorType LandscapeEditorDrawSystem::EnableTilemaskEditing()
{
    eErrorType initError = Init();
    if (initError != LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS)
    {
        return initError;
    }

    landscapeProxy->SetMode(LandscapeProxy::MODE_ORIGINAL_LANDSCAPE);
    return LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS;
}

void LandscapeEditorDrawSystem::DisableTilemaskEditing()
{
}

LandscapeEditorDrawSystem::eErrorType LandscapeEditorDrawSystem::Init()
{
    if (heightmapProxy == nullptr)
    {
        DAVA::Heightmap* heightmap = baseLandscape->GetHeightmap();
        if ((heightmap == nullptr) || (heightmap->Size() == 0))
        {
            return LANDSCAPE_EDITOR_SYSTEM_HEIGHTMAP_ABSENT;
        }
        DAVA::ScopedPtr<DAVA::Heightmap> clonedHeightmap(heightmap->Clone(nullptr));
        heightmapProxy = new HeightmapProxy(clonedHeightmap);
    }

    if (customColorsProxy == nullptr)
    {
        customColorsProxy = new CustomColorsProxy(DAVA::Landscape::CUSTOM_COLOR_TEXTURE_SIZE);
    }

    if (rulerToolProxy == nullptr)
    {
        rulerToolProxy = new RulerToolProxy(static_cast<DAVA::int32>(GetTextureSize(DAVA::Landscape::TEXTURE_COLOR)));
    }

    return LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS;
}

LandscapeEditorDrawSystem::eErrorType LandscapeEditorDrawSystem::InitLandscape(DAVA::Entity* landscapeEntity, DAVA::Landscape* landscape)
{
    DeinitLandscape();

    if (!landscapeEntity || !landscape)
    {
        return LANDSCAPE_EDITOR_SYSTEM_LANDSCAPE_ENTITY_ABSENT;
    }

    landscapeNode = landscapeEntity;
    baseLandscape = SafeRetain(landscape);

    UpdateTilemaskPathname();

    DVASSERT(landscapeProxy == nullptr);
    landscapeProxy = new LandscapeProxy(baseLandscape, landscapeNode);

    return LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS;
}

void LandscapeEditorDrawSystem::DeinitLandscape()
{
    landscapeNode = NULL;
    SafeRelease(landscapeProxy);
    SafeRelease(baseLandscape);
}

void LandscapeEditorDrawSystem::ClampToTexture(const DAVA::FastName& level, DAVA::Rect& rect)
{
    GetTextureRect(level).ClampToRect(rect);
}

void LandscapeEditorDrawSystem::ClampToHeightmap(DAVA::Rect& rect)
{
    GetHeightmapRect().ClampToRect(rect);
}

void LandscapeEditorDrawSystem::AddEntity(DAVA::Entity* entity)
{
    if (IsSystemEnabled() == false)
    {
        return;
    }

    DAVA::Landscape* landscape = GetLandscape(entity);
    if (landscape != NULL)
    {
        entity->SetLocked(true);

        InitLandscape(entity, landscape);
    }
}

void LandscapeEditorDrawSystem::RemoveEntity(DAVA::Entity* entity)
{
    if (entity == landscapeNode && IsSystemEnabled())
    {
        SceneEditor2* sceneEditor = static_cast<SceneEditor2*>(GetScene());

        bool needRemoveBaseLandscape = sceneEditor->IsToolsEnabled(SceneEditor2::LANDSCAPE_TOOLS_ALL
                                                                   & ~SceneEditor2::LANDSCAPE_TOOL_TILEMAP_EDITOR);

        sceneEditor->DisableToolsInstantly(SceneEditor2::LANDSCAPE_TOOLS_ALL);

        if (needRemoveBaseLandscape)
        {
            sceneEditor->renderUpdateSystem->RemoveEntity(entity);
        }

        DeinitLandscape();

        DAVA::Entity* landEntity = FindLandscapeEntity(sceneEditor);
        if (landEntity != nullptr && landEntity != entity)
        {
            InitLandscape(landEntity, GetLandscape(landEntity));
        }
    }
}

void LandscapeEditorDrawSystem::PrepareForRemove()
{
    DeinitLandscape();
}

bool LandscapeEditorDrawSystem::SaveTileMaskTexture()
{
    if (baseLandscape == nullptr || !GetLandscapeProxy()->IsTilemaskChanged())
    {
        return false;
    }

    DAVA::Texture* texture = GetTileMaskTexture();
    if (texture != nullptr)
    {
        DAVA::Image* image = texture->CreateImageFromMemory();

        if (image)
        {
            DAVA::ImageSystem::Save(sourceTilemaskPath, image);
            SafeRelease(image);
        }

        GetLandscapeProxy()->ResetTilemaskChanged();

        return true;
    }

    return false;
}

void LandscapeEditorDrawSystem::ResetTileMaskTexture()
{
    if (baseLandscape == nullptr)
    {
        return;
    }

    DAVA::ScopedPtr<DAVA::Texture> texture(DAVA::Texture::CreateFromFile(sourceTilemaskPath));
    texture->Reload();
    SetTileMaskTexture(texture);
}

void LandscapeEditorDrawSystem::SetTileMaskTexture(DAVA::Texture* texture)
{
    if (baseLandscape == nullptr)
    {
        return;
    }

    DAVA::NMaterial* landscapeMaterial = baseLandscape->GetMaterial();
    while (landscapeMaterial != nullptr)
    {
        if (landscapeMaterial->HasLocalTexture(DAVA::Landscape::TEXTURE_TILEMASK))
            break;

        landscapeMaterial = landscapeMaterial->GetParent();
    }

    if (landscapeMaterial != nullptr)
    {
        landscapeMaterial->SetTexture(DAVA::Landscape::TEXTURE_TILEMASK, texture);
    }
}

DAVA::Texture* LandscapeEditorDrawSystem::GetTileMaskTexture()
{
    if (baseLandscape != nullptr)
    {
        DAVA::NMaterial* landscapeMaterial = baseLandscape->GetMaterial();
        if (landscapeMaterial != nullptr)
        {
            return landscapeMaterial->GetEffectiveTexture(DAVA::Landscape::TEXTURE_TILEMASK);
        }
    }

    return nullptr;
}

LandscapeEditorDrawSystem::eErrorType LandscapeEditorDrawSystem::VerifyLandscape() const
{
    //landscape initialization should be handled by AddEntity/RemoveEntity methods
    if (!landscapeNode || !baseLandscape || !landscapeProxy)
    {
        return LANDSCAPE_EDITOR_SYSTEM_LANDSCAPE_ENTITY_ABSENT;
    }

    DAVA::Texture* tileMask = landscapeProxy->GetLandscapeTexture(DAVA::Landscape::TEXTURE_TILEMASK);
    if (tileMask == nullptr || tileMask->IsPinkPlaceholder())
    {
        return LANDSCAPE_EDITOR_SYSTEM_TILEMASK_TEXTURE_ABSENT;
    }

    DAVA::Texture* texTile = baseLandscape->GetMaterial()->GetEffectiveTexture(DAVA::Landscape::TEXTURE_TILE);
    if ((texTile == nullptr || texTile->IsPinkPlaceholder()))
    {
        return LANDSCAPE_EDITOR_SYSTEM_TILE_TEXTURE_ABSENT;
    }

    DAVA::Texture* texColor = baseLandscape->GetMaterial()->GetEffectiveTexture(DAVA::Landscape::TEXTURE_COLOR);
    if ((texColor == nullptr || texColor->IsPinkPlaceholder()))
    {
        return LANDSCAPE_EDITOR_SYSTEM_COLOR_TEXTURE_ABSENT;
    }

    return LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS;
}

DAVA::Landscape* LandscapeEditorDrawSystem::GetBaseLandscape() const
{
    return baseLandscape;
}

DAVA::String LandscapeEditorDrawSystem::GetDescriptionByError(eErrorType error)
{
    DAVA::String ret;
    switch (error)
    {
    case LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS:
        ret = ResourceEditor::LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS;
        break;
    case LANDSCAPE_EDITOR_SYSTEM_LANDSCAPE_ENTITY_ABSENT:
        ret = ResourceEditor::LANDSCAPE_EDITOR_SYSTEM_LANDSCAPE_ENTITY_ABSENT;
        break;
    case LANDSCAPE_EDITOR_SYSTEM_TILEMASK_TEXTURE_ABSENT:
        ret = ResourceEditor::LANDSCAPE_EDITOR_SYSTEM_TILEMASK_TEXTURE_ABSETN;
        break;
    case LANDSCAPE_EDITOR_SYSTEM_FULLTILED_TEXTURE_ABSENT:
        ret = ResourceEditor::LANDSCAPE_EDITOR_SYSTEM_FULLTILED_TEXTURE_ABSETN;
        break;
    case LANDSCAPE_EDITOR_SYSTEM_TILE_TEXTURE_ABSENT:
        ret = ResourceEditor::LANDSCAPE_EDITOR_SYSTEM_TILE_TEXTURE_ABSENT;
        break;
    case LANDSCAPE_EDITOR_SYSTEM_COLOR_TEXTURE_ABSENT:
        ret = ResourceEditor::LANDSCAPE_EDITOR_SYSTEM_COLOR_TEXTURE_ABSENT;
        break;
    case LANDSCAPE_EDITOR_SYSTEM_HEIGHTMAP_ABSENT:
        ret = ResourceEditor::LANDSCAPE_EDITOR_SYSTEM_HEIGHTMAP_ABSENT;
        break;
    case LANDSCAPE_EDITOR_SYSTEM_CUSTOMCOLORS_ABSENT:
        ret = ResourceEditor::LANDSCAPE_EDITOR_SYSTEM_CUSTOMCOLORS_ABSENT;
        break;

    default:
        DVASSERT(false && "Unknown error");
        break;
    }
    return ret;
}

void LandscapeEditorDrawSystem::ProcessCommand(const RECommandNotificationObject& commandNotification)
{
    static const DAVA::FastName heightmapPath("heightmapPath");

    if (commandNotification.MatchCommandIDs({ CMDID_INSP_MEMBER_MODIFY, CMDID_INSP_DYNAMIC_MODIFY, CMDID_REFLECTED_FIELD_MODIFY }))
    {
        auto processSingleCommand = [this](const RECommand* command, bool redo) {
            if (command->MatchCommandID(CMDID_INSP_MEMBER_MODIFY))
            {
                const InspMemberModifyCommand* cmd = static_cast<const InspMemberModifyCommand*>(command);
                if (heightmapPath == cmd->member->Name() && baseLandscape != nullptr)
                {
                    DAVA::Heightmap* heightmap = baseLandscape->GetHeightmap();
                    if ((heightmap != nullptr) && (heightmap->Size() > 0))
                    {
                        DAVA::ScopedPtr<DAVA::Heightmap> clonedHeightmap(heightmap->Clone(nullptr));
                        SafeRelease(heightmapProxy);
                        heightmapProxy = new HeightmapProxy(clonedHeightmap);

                        DAVA::float32 size = static_cast<DAVA::float32>(heightmapProxy->Size());
                        heightmapProxy->UpdateRect(DAVA::Rect(0.f, 0.f, size, size));
                    }
                }
            }
            else if (command->MatchCommandID(CMDID_INSP_DYNAMIC_MODIFY))
            {
                const InspDynamicModifyCommand* cmd = static_cast<const InspDynamicModifyCommand*>(command);
                if (DAVA::Landscape::TEXTURE_TILEMASK == cmd->key)
                {
                    UpdateTilemaskPathname();
                }
            }
            else if (command->MatchCommandID(CMDID_REFLECTED_FIELD_MODIFY))
            {
                const SetFieldValueCommand* cmd = static_cast<const SetFieldValueCommand*>(command);
                if (heightmapPath == cmd->GetField().key.Cast<DAVA::FastName>(DAVA::FastName("")) && baseLandscape != nullptr)
                {
                    DAVA::Heightmap* heightmap = baseLandscape->GetHeightmap();
                    if ((heightmap != nullptr) && (heightmap->Size() > 0))
                    {
                        DAVA::ScopedPtr<DAVA::Heightmap> clonedHeightmap(heightmap->Clone(nullptr));
                        SafeRelease(heightmapProxy);
                        heightmapProxy = new HeightmapProxy(clonedHeightmap);

                        DAVA::float32 size = static_cast<DAVA::float32>(heightmapProxy->Size());
                        heightmapProxy->UpdateRect(DAVA::Rect(0.f, 0.f, size, size));
                    }
                }
            }
        };

        commandNotification.ExecuteForAllCommands(processSingleCommand);
    }
}

bool LandscapeEditorDrawSystem::UpdateTilemaskPathname()
{
    if (nullptr != baseLandscape)
    {
        auto texture = baseLandscape->GetMaterial()->GetEffectiveTexture(DAVA::Landscape::TEXTURE_TILEMASK);
        if (nullptr != texture)
        {
            DAVA::FilePath path = texture->GetDescriptor()->GetSourceTexturePathname();
            if (path.GetType() == DAVA::FilePath::PATH_IN_FILESYSTEM)
            {
                sourceTilemaskPath = path;
            }
            return true;
        }
    }

    return false;
}

bool LandscapeEditorDrawSystem::InitTilemaskImageCopy()
{
    DVASSERT(landscapeProxy != nullptr);
    return landscapeProxy->InitTilemaskImageCopy(sourceTilemaskPath);
}
