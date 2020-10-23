#pragma once

#include "LandscapeEditorSystem.h"
#include "LandscapeEditorDrawSystem.h"

class RulerToolSystem : public LandscapeEditorSystem
{
    static const DAVA::int32 APPROXIMATION_COUNT = 10;

public:
    RulerToolSystem(DAVA::Scene* scene);
    virtual ~RulerToolSystem();

    LandscapeEditorDrawSystem::eErrorType EnableLandscapeEditing();
    bool DisableLandscapeEdititing();

    void PrepareForRemove() override;
    void Process(DAVA::float32 timeElapsed) override;
    bool Input(DAVA::UIEvent* event) override;

    DAVA::float32 GetLength();
    DAVA::float32 GetPreviewLength();

protected:
    DAVA::Vector2 MirrorPoint(const DAVA::Vector2& point) const;

    DAVA::uint32 curToolSize;
    DAVA::Texture* toolImageTexture;

    DAVA::List<DAVA::Vector2> linePoints;
    DAVA::List<DAVA::float32> lengths;
    DAVA::Vector2 previewPoint;
    DAVA::float32 previewLength;
    bool previewEnabled;

    void SetStartPoint(const DAVA::Vector2& point);
    void AddPoint(const DAVA::Vector2& point);
    void RemoveLastPoint();
    void CalcPreviewPoint(const DAVA::Vector2& point, bool force = false);
    DAVA::float32 GetLength(const DAVA::Vector2& startPoint, const DAVA::Vector2& endPoint);
    void DrawPoints();
    void DisablePreview();
    void SendUpdatedLength();

    void ClearInternal();
};
