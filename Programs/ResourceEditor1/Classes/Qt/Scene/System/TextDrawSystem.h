#pragma once

#include "CameraSystem.h"
#include "Classes/Qt/Scene/System/EditorSceneSystem.h"

// framework
#include "Entity/SceneSystem.h"
#include "Render/2D/GraphicFont.h"

namespace DAVA
{
class NMaterial;
}

class TextDrawSystem : public DAVA::SceneSystem, public EditorSceneSystem
{
public:
    enum class Align : DAVA::uint8
    {
        TopLeft,
        TopCenter,
        TopRight,
        Left,
        Center,
        Right,
        BottomLeft,
        BottomCenter,
        BottomRight
    };

public:
    TextDrawSystem(DAVA::Scene* scene, SceneCameraSystem* cameraSystem);
    ~TextDrawSystem();

    void PrepareForRemove() override;
    DAVA::Vector2 ToPos2d(const DAVA::Vector3& pos3d) const;

    void DrawText(const DAVA::Vector2& pos2d, const DAVA::String& text, const DAVA::Color& color, Align align = Align::TopLeft);
    void DrawText(const DAVA::Vector2& pos2d, const DAVA::WideString& text, const DAVA::Color& color, DAVA::float32 fontSize, Align align = Align::TopLeft);

    DAVA::GraphicFont* GetFont() const;

protected:
    void Draw() override;

    struct TextToDraw
    {
        TextToDraw(const DAVA::Vector2& pos_, const DAVA::WideString& text_, const DAVA::Color& color_, Align align_, DAVA::float32 fontSize_)
            : pos(pos_)
            , text(text_)
            , color(color_)
            , align(align_)
            , fontSize(fontSize_)
        {
        }

        DAVA::Vector2 pos;
        DAVA::WideString text;
        DAVA::Color color;
        Align align = Align::TopLeft;
        DAVA::float32 fontSize = 10.f;
    };

    using GraphicFontVertexVector = DAVA::Vector<DAVA::GraphicFont::GraphicFontVertex>;

    void AdjustPositionBasedOnAlign(DAVA::float32& x, DAVA::float32& y, const DAVA::Size2i& size, Align align);
    void PushNextBatch(const DAVA::Color& color);

private:
    SceneCameraSystem* cameraSystem = nullptr;
    DAVA::GraphicFont* font = nullptr;
    DAVA::NMaterial* fontMaterial = nullptr;
    DAVA::Vector<TextToDraw> textToDraw;
    GraphicFontVertexVector vertices;
};

inline DAVA::GraphicFont* TextDrawSystem::GetFont() const
{
    return font;
}
