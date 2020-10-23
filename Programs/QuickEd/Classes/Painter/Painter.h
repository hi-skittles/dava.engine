#pragma once

#include "Classes/EditorSystems/EditorSystemsConstants.h"

#include <Base/RefPtr.h>
#include <Base/BaseTypes.h>
#include <Math/Color.h>
#include <Render/2D/GraphicFont.h>
#include <Render/Material/NMaterial.h>
#include <UI/UIGeometricData.h>

namespace DAVA
{
class Window;
class NMaterial;
}

namespace Painting
{
using namespace DAVA;

struct DrawTextParams
{
    String text;
    float32 textSize = 12.0f;
    //text color
    Color color = Color::Black;
    //position of item in screen coordinates
    Vector2 pos = Vector2(0.0f, 0.0f);
    //direction to draw text item relative to it position.
    //As an example if direction is equal to ALIGN_LEFT item X position will be equal to params.pos.x - params.size.x - margin.x
    int32 direction = ALIGN_RIGHT | ALIGN_BOTTOM;
    //margin between params position and actual text item position.
    //As an example if direction is equal to ALIGN_LEFT item X position will be equal to params.pos.x - params.size.x - margin.x
    Vector2 margin = Vector2(0.0f, 0.0f);
};

struct DrawLineParams
{
    //line color
    Color color = Color::Black;
    //position of item in screen coordinates
    Vector2 startPos = Vector2(0.0f, 0.0f);
    //size of item in screen coordinates. If size is zero it will be calculated from text metrics
    Vector2 endPos = Vector2(0.0f, 0.0f);

    //line width in pixels
    float32 width = 1.0f;

    enum eType
    {
        SOLID,
        DOT
    };

    eType type = SOLID;
};

class Painter final
{
public:
    Painter();

    void Draw(eSystems order, DrawTextParams params);
    void Draw(eSystems order, const DrawLineParams& params);

    void OnFrame(Window* window);

private:
    void OnFrame(const DrawTextParams& params);
    void OnFrame(const DrawLineParams& params);

    void ApplyParamPos(DrawTextParams& params) const;

    using GraphicFontVertexVector = Vector<GraphicFont::GraphicFontVertex>;

    RefPtr<GraphicFont> font;
    RefPtr<NMaterial> fontMaterial;
    RefPtr<NMaterial> textureMaterial;
    GraphicFontVertexVector vertices;

    struct DrawItems
    {
        Vector<DrawTextParams> drawTextItems;
        Vector<DrawLineParams> drawLineItems;
    };

    Map<eSystems, DrawItems> drawItems;

    float32 cachedSpread = 0.0f;
};
}
