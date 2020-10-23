#pragma once

#include "LandscapeEditorSystem.h"


#include "Commands2/MetaObjModifyCommand.h"
#include "LandscapeEditorDrawSystem.h"
#include "Classes/Qt/Scene/System/EditorSceneSystem.h"

#include "Render/UniqueStateSet.h"

class TilemaskEditorSystem : public LandscapeEditorSystem
{
public:
    enum eTilemaskDrawType
    {
        TILEMASK_DRAW_NORMAL = 0,
        TILEMASK_DRAW_COPY_PASTE,

        TILEMASK_DRAW_TYPES_COUNT
    };

    TilemaskEditorSystem(DAVA::Scene* scene);
    virtual ~TilemaskEditorSystem();

    LandscapeEditorDrawSystem::eErrorType EnableLandscapeEditing();
    bool DisableLandscapeEdititing();

    void PrepareForRemove() override
    {
    }
    void Process(DAVA::float32 timeElapsed) override;
    bool Input(DAVA::UIEvent* event) override;
    void InputCancelled(DAVA::UIEvent* event) override;

    void SetBrushSize(DAVA::int32 brushSize);
    DAVA::int32 GetBrushSize();
    void SetStrength(DAVA::float32 strength);
    DAVA::float32 GetStrength();
    void SetToolImage(const DAVA::FilePath& toolImagePath, DAVA::int32 index);
    DAVA::int32 GetToolImage();
    void SetTileTexture(DAVA::uint32 tileTexture);
    DAVA::uint32 GetTileTextureIndex();

    DAVA::uint32 GetTileTextureCount() const;
    DAVA::Texture* GetTileTexture();
    DAVA::Color GetTileColor(DAVA::uint32 index);
    void SetTileColor(DAVA::int32 index, const DAVA::Color& color);

    void SetDrawingType(eTilemaskDrawType type);
    eTilemaskDrawType GetDrawingType();

protected:
    void Draw() override;

    DAVA::uint32 curToolSize;

    DAVA::Texture* toolImageTexture;
    DAVA::Texture* landscapeTilemaskTexture;

    DAVA::uint32 tileTextureNum;

    DAVA::NMaterial* editorMaterial;

    eTilemaskDrawType drawingType;
    eTilemaskDrawType activeDrawingType;
    DAVA::float32 strength;
    DAVA::FilePath toolImagePath;
    DAVA::int32 toolImageIndex;

    rhi::Packet quadPacket;
    DAVA::uint32 quadVertexLayoutID;

    DAVA::Vector2 copyPasteFrom;
    DAVA::Vector2 copyPasteOffset;

    DAVA::Rect updatedRectAccumulator;

    bool editingIsEnabled;

    DAVA::Texture* toolTexture;
    bool toolSpriteUpdated;

    bool needCreateUndo;

    const DAVA::FastName& textureLevel;

    void UpdateBrushTool();
    void UpdateToolImage();

    void AddRectToAccumulator(const DAVA::Rect& rect);
    void ResetAccumulatorRect();
    DAVA::Rect GetUpdatedRect();

    void CreateMaskTexture();

    void CreateUndoPoint();

    void InitSprites();

    void FinishEditing();
};
