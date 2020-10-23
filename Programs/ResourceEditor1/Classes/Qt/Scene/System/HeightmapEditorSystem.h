#ifndef __RESOURCEEDITORQT__HEIGHTMAPEDITORSYSTEM__
#define __RESOURCEEDITORQT__HEIGHTMAPEDITORSYSTEM__

#include "LandscapeEditorSystem.h"
#include "LandscapeEditorDrawSystem.h"

#include "Render/UniqueStateSet.h"

class HoodSystem;

class HeightmapEditorSystem : public LandscapeEditorSystem
{
public:
    enum eHeightmapDrawType
    {
        HEIGHTMAP_DRAW_ABSOLUTE = 0,
        HEIGHTMAP_DRAW_RELATIVE,
        HEIGHTMAP_DRAW_AVERAGE,
        HEIGHTMAP_DRAW_ABSOLUTE_DROPPER,
        HEIGHTMAP_DROPPER,
        HEIGHTMAP_COPY_PASTE,

        HEIGHTMAP_DRAW_TYPES_COUNT
    };

    HeightmapEditorSystem(DAVA::Scene* scene);
    virtual ~HeightmapEditorSystem();

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
    void SetAverageStrength(DAVA::float32 averageStrength);
    DAVA::float32 GetAverageStrength();
    void SetToolImage(const DAVA::FilePath& toolImagePath, DAVA::int32 index);
    DAVA::int32 GetToolImageIndex();
    void SetDrawingType(eHeightmapDrawType type);
    eHeightmapDrawType GetDrawingType();

    void SetDropperHeight(DAVA::float32 height);
    DAVA::float32 GetDropperHeight();

protected:
    DAVA::Vector2 GetHeightmapPositionFromCursor() const;

protected:
    DAVA::Texture* squareTexture = nullptr;
    DAVA::uint32 curToolSize = 30;
    DAVA::Image* curToolImage = nullptr;

    eHeightmapDrawType drawingType = HEIGHTMAP_DRAW_ABSOLUTE;
    DAVA::float32 strength = 15.f;
    DAVA::float32 averageStrength = 0.5f;
    bool inverseDrawingEnabled = false;
    DAVA::FilePath toolImagePath;
    DAVA::int32 toolImageIndex = 0;

    DAVA::float32 curHeight = 0.f;
    DAVA::Vector2 copyPasteFrom;
    DAVA::Vector2 copyPasteTo;

    DAVA::Rect heightmapUpdatedRect;

    bool editingIsEnabled = false;

    DAVA::Heightmap* originalHeightmap = nullptr;

    eHeightmapDrawType activeDrawingType;

    void UpdateToolImage();
    void UpdateBrushTool(DAVA::float32 timeElapsed);

    void AddRectToAccumulator(DAVA::Rect& accumulator, const DAVA::Rect& rect);
    void ResetAccumulatorRect(DAVA::Rect& accumulator);
    DAVA::Rect GetHeightmapUpdatedRect();

    void StoreOriginalHeightmap();
    void FinishEditing(bool applyModification);
};

#endif /* defined(__RESOURCEEDITORQT__HEIGHTMAPEDITORSYSTEM__) */
