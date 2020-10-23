#pragma once

#include "Base/BaseTypes.h"
#include "Base/Singleton.h"
#include "Functional/Function.h"
#include "Render/2D/Sprite.h"
#include "Render/2D/Systems/BatchDescriptor2D.h"
#include "Render/RenderBase.h"

namespace DAVA
{
class Font;
class Sprite;
class TextBlock;
class UIGeometricData;

struct TiledDrawData
{
    struct Unit
    {
        Vector<Vector2> vertices;
        Vector<Vector2> texCoords;
        Vector<Vector2> transformedVertices;
        Vector<uint16> indeces;
    };
    Vector<Unit> units;

    void GenerateTileData();
    void GenerateAxisData(float32 size, float32 spriteSize, float32 textureSize, float32 stretchCap, Vector<Vector3>& axisData);
    void GenerateTransformData();

    Sprite* sprite;
    Texture* texture;
    int32 frame;
    Vector2 size;
    Vector2 stretchCap;
    Matrix3 transformMatr;
};

struct StretchDrawData
{
    Vector<Vector2> vertices;
    Vector<Vector2> transformedVertices;
    Vector<Vector2> texCoords;
    static const uint16 indeces[18 * 3];

    void GenerateStretchData();
    void GenerateTransformData();
    uint32 GetVertexInTrianglesCount() const;

    Sprite* sprite;
    Texture* texture;
    int32 frame;
    Vector2 size;
    int32 type;
    Vector2 stretchCap;
    Matrix3 transformMatr;
    bool usePerPixelAccuracy;
};

struct TiledMultilayerData
{
    Vector<Vector2> vertices;
    Vector<Vector2> transformedVertices;
    Vector<Vector2> texCoordsMask;
    Vector<Vector2> texCoordsDetail;
    Vector<Vector2> texCoordsGradient;
    Vector<Vector2> texCoordsContour;
    Vector<uint16> indices;

    rhi::HTextureSet textureSet;
    rhi::HSamplerState samplerState;

    Sprite* mask = nullptr;
    Sprite* detail = nullptr;
    Sprite* gradient = nullptr;
    Sprite* contour = nullptr;

    Texture* mask_texture = nullptr;
    Texture* detail_texture = nullptr;
    Texture* gradient_texture = nullptr;
    Texture* contour_texture = nullptr;

    Vector2 size;
    Vector2 stretchCap;
    Matrix3 transformMatr;
    bool usePerPixelAccuracy;

    void GenerateTileData();
    void GenerateTransformData(bool usePerPixelAccuracy);
    ~TiledMultilayerData();

private:
    struct AxisData
    {
        float32 pos;
        float32 texCoordsMask;
        float32 texCoordsDetail;
        float32 texCoordsGradient;
        float32 texCoordsContour;
    };

    struct SingleStretchData
    {
        Vector2 uvBase;
        Vector2 uvCapMin;
        Vector2 uvCapMax;
        Vector2 uvMax;
    };

    SingleStretchData GenerateStretchData(Sprite* sprite);
    Vector<AxisData> GenerateSingleAxisData(float32 inSize, float32 inTileSize, float32 inStratchCap,
                                            float32 gradientBase, float32 gradientDelta, float32 detailBase, float32 detailDelta,
                                            float32 contourBase, float32 contourStretchBase, float32 contourStretchMax, float32 contourMax,
                                            float32 maskBase, float32 maskStretchBase, float32 maskStretchMax, float32 maskMax); //unlike in TileData, this method generates actual vertices info along the axis
};

class RenderSystem2D : public Singleton<RenderSystem2D>
{
public:
    struct RenderTargetPassDescriptor
    {
        rhi::HTexture colorAttachment;
        rhi::HTexture depthAttachment;
        uint32 width = 0;
        uint32 height = 0;
        PixelFormat format = PixelFormat::FORMAT_INVALID;
        Color clearColor = Color::Clear;
        int32 priority = 0; // PRIORITY_SERVICE_2D;
        bool transformVirtualToPhysical = true;
        bool clearTarget = true;
    };

    enum ColorOperations
    {
        COLOR_MUL = 0,
        COLOR_ADD,
        ALPHA_MUL,
        ALPHA_ADD,
    };

    static const FastName RENDER_PASS_NAME;
    static const FastName FLAG_COLOR_OP;
    static const FastName FLAG_GRADIENT_MODE;

    static NMaterial* DEFAULT_2D_COLOR_MATERIAL;
    static NMaterial* DEFAULT_2D_TEXTURE_MATERIAL;
    static NMaterial* DEFAULT_2D_TEXTURE_PREMULTIPLIED_ALPHA_MATERIAL;
    static NMaterial* DEFAULT_2D_TEXTURE_ADDITIVE_MATERIAL;
    static NMaterial* DEFAULT_2D_TEXTURE_NOBLEND_MATERIAL;
    static NMaterial* DEFAULT_2D_TEXTURE_ALPHA8_MATERIAL;
    static NMaterial* DEFAULT_2D_TEXTURE_GRAYSCALE_MATERIAL;
    static NMaterial* DEFAULT_2D_FILL_ALPHA_MATERIAL;
    static NMaterial* DEFAULT_COMPOSIT_MATERIAL[GRADIENT_BLEND_MODE_COUNT];

    RenderSystem2D();
    virtual ~RenderSystem2D();

    void Init();

    void Draw(Sprite* sprite, SpriteDrawState* drawState, const Color& color);
    void DrawStretched(Sprite* sprite, SpriteDrawState* drawState, Vector2 streatchCap, int32 type, const UIGeometricData& gd, StretchDrawData** pStreachData, const Color& color);
    void DrawTiled(Sprite* sprite, SpriteDrawState* drawState, const Vector2& streatchCap, const UIGeometricData& gd, TiledDrawData** pTiledData, const Color& color);
    void DrawTiledMultylayer(Sprite* mask, Sprite* detail, Sprite* gradient, Sprite* contour,
                             SpriteDrawState* state, const Vector2& stretchCapVector, const UIGeometricData& gd, TiledMultilayerData** pTileData, const Color& color);

    void SetViewMatrix(const Matrix4& viewMatrix);

    /**
     * Destroy current buffers and create new.
     * @param verticesCount vertices count per buffer (size of buffer equals verticesCount*GetVertexSize(vertexFormat))
     * @param indicesCount indices count per buffer (size of buffer equals indicesCount*sizeof(uint16))
     * @param buffersCount buffers count
     */
    void HardResetBatchingBuffers(uint32 verticesCount, uint32 indicesCount, uint8 buffersCount);

    void PushBatch(const BatchDescriptor2D& batchDesc);

    /*
     *  note - it will flush currently batched!
     *  it will also modify packet to add current clip
     */
    void DrawPacket(rhi::Packet& packet);
    /*!
     * Highlight controls which has vertices count bigger than verticesCount.
     * Work only with RenderOptions::HIGHLIGHT_BIG_CONTROLS option enabled.
     * @param verticesCount vertices limit
     */
    void SetHightlightControlsVerticesLimit(uint32 verticesCount);

    void BeginFrame();
    void EndFrame();
    void Flush();

    void SetClip(const Rect& rect);
    void IntersectClipRect(const Rect& rect);
    void RemoveClip();

    void PushClip();
    void PopClip();

    void ScreenSizeChanged();

    void SetSpriteClipping(bool clipping);
    bool GetSpriteClipping() const;

    void BeginRenderTargetPass(Texture* target, bool needClear = true, const Color& clearColor = Color::Clear, int32 priority = PRIORITY_SERVICE_2D);
    void BeginRenderTargetPass(const RenderTargetPassDescriptor&);
    void EndRenderTargetPass();

    /* 2D DRAW HELPERS */

    /**
    \brief Draws line from pt1 to pt2
    \param pt1 starting point
    \param pt2 ending point
    \param color draw color
    */
    void DrawLine(const Vector2& pt1, const Vector2& pt2, const Color& color);

    /**
    \brief Draws line from pt1 to pt2
    \param pt1 starting point
    \param pt2 ending point
    \param color draw color
    */
    void DrawLine(const Vector2& start, const Vector2& end, float32 lineWidth, const Color& color);

    /**
    \brief Draws multiple lines.
    \param linePoints list of points in the format (startX, startY, endX, endY), (startX, startY, endX, endY)...
    \param color draw color
    */
    void DrawLines(const Vector<float32>& linePoints, const Color& color);

    /**
    \brief Draws given rect in 2D space
    \param pt1 starting point
    \param pt2 ending point
    \param color draw color
    */
    void DrawRect(const Rect& rect, const Color& color);

    /**
    \brief Fills given rect in 2D space
    \param pt1 starting point
    \param pt2 ending point
    \param color draw color
    */
    void FillRect(const Rect& rect, const Color& color, NMaterial* material = DEFAULT_2D_COLOR_MATERIAL);

    /**
    \brief Fills given rect in 2D space using four colors in corners
    \param pt1 starting point
    \param pt2 ending point
    \param xy top left color
    \param wy top right color
    \param xh bottom left color
    \param wh bottom right color
    */
    void FillGradientRect(const Rect& rect, const Color& xy, const Color& wy, const Color& xh, const Color& wh);

    /**
    \brief Draws grid in the given rect
    \param rect rect to fill grid with
    \param gridSize distance between grid lines
    \param color grid color
    */
    void DrawGrid(const Rect& rect, const Vector2& gridSize, const Color& color);

    /**
    \brief Draws circle in 2D space
    \param center center of the circle
    \param radius radius of the circle
    \param color draw color
    */
    void DrawCircle(const Vector2& center, float32 radius, const Color& color);

    /**
    \brief Draws all concecutive lines from given polygon
    \param polygon the polygon we want to draw
    \param closed you should set this flag to true if you want to connect last point of polygon with first point
    \param color draw color
    */
    void DrawPolygon(const Polygon2& polygon, bool closed, const Color& color);

    /**
    \brief Fill convex polygon with color. As all other draw functions this function use global color that can be set with RenderSystem2D::Instance()->SetColor function.
    \param polygon the polygon we want to draw
    \param color draw color
    */
    void FillPolygon(const Polygon2& polygon, const Color& color);

    /**
    \brief Draws all concecutive lines from given polygon after transformation
    \param polygon the polygon we want to draw
    \param closed you should set this flag to true if you want to connect last point of polygon with first point
    \param transform transform that will be applied to polygon before it will be drawn
    \param color draw color
    */
    void DrawPolygonTransformed(const Polygon2& polygon, bool closed, const Matrix3& transform, const Color& color);

    void DrawTexture(Texture* texture, NMaterial* material, const Color& color,
                     const Rect& dstRect = Rect(0.f, 0.f, -1.f, -1.f), const Rect& srcRect = Rect(0.f, 0.f, -1.f, -1.f));

    void DrawTextureWithoutAdjustingRects(Texture* texture, NMaterial* material, const Color& color, const Rect& dstRect, const Rect& srcRect);

    const RenderTargetPassDescriptor& GetActiveTargetDescriptor();
    const RenderTargetPassDescriptor& GetMainTargetDescriptor();
    void SetMainTargetDescriptor(const RenderTargetPassDescriptor& descriptor);

    Vector2 GetAlignedVertex(const Vector2& vertex);

private:
    void UpdateVirtualToPhysicalMatrix(bool);
    bool IsPreparedSpriteOnScreen(SpriteDrawState* drawState);
    void Setup2DMatrices();

    void AddPacket(rhi::Packet& packet);

    Rect TransformClipRect(const Rect& rect, const Matrix4& transformMatrix);

    inline bool IsRenderTargetPass()
    {
        return (currentPacketListHandle != packetList2DHandle);
    }

    const Matrix4& VirtualToPhysicalMatrix() const;

    float32 AlignToX(float32 value);
    float32 AlignToY(float32 value);

    uint32 GetVertexLayoutId(uint32 texCoordStreamCount);
    uint32 GetVBOStride(uint32 texCoordStreamCount);

private:
    Matrix4 currentVirtualToPhysicalMatrix;
    Vector2 currentPhysicalToVirtualScale;

    Matrix4 actualVirtualToPhysicalMatrix;
    Vector2 actualPhysicalToVirtualScale;

    Matrix4 projMatrix;
    Matrix4 viewMatrix;
    uint32 projMatrixSemantic = 0;
    uint32 viewMatrixSemantic = 0;
    std::stack<Rect> clipStack;
    Rect currentClip;

    Array<float32, 8> spriteTempVertices;
    Array<uint32, 4> spriteTempColors;
    Vector<Vector2> spriteClippedTexCoords;
    Vector<Vector2> spriteClippedVertices;

    int32 spriteVertexCount = 0;
    int32 spriteIndexCount = 0;

    SpriteDrawState defaultSpriteDrawState;

    bool spriteClipping = true;

    Vector<uint8> currentVertexBuffer;
    Vector<uint16> currentIndexBuffer;
    rhi::Packet currentPacket;
    uint32 currentTexcoordStreamCount = 1; //1 is for default draw
    uint32 currentIndexBase = 0;
    uint32 vertexIndex = 0;
    uint32 indexIndex = 0;
    NMaterial* lastMaterial = nullptr;
    Rect lastClip;
    Matrix4 lastCustomWorldMatrix;
    bool lastUsedCustomWorldMatrix = false;
    float32 globalTime = 0.f;

    uint32 VBO_STRIDE[BatchDescriptor2D::MAX_TEXTURE_STREAMS_COUNT + 1];
    uint32 vertexLayouts2d[BatchDescriptor2D::MAX_TEXTURE_STREAMS_COUNT + 1];

    // Batching errors handling
    enum ErrorFlag
    {
        NO_ERRORS = 0,
        BUFFER_OVERFLOW_ERROR = 1,
    };
    uint32 prevFrameErrorsFlags = NO_ERRORS;
    uint32 currFrameErrorsFlags = NO_ERRORS;
    uint32 highlightControlsVerticesLimit = 0;

    rhi::HRenderPass pass2DHandle;
    rhi::HPacketList packetList2DHandle;
    rhi::HRenderPass passTargetHandle;
    rhi::HPacketList currentPacketListHandle;

    RenderTargetPassDescriptor mainTargetDescriptor;
    RenderTargetPassDescriptor renderPassTargetDescriptor;
};

inline void RenderSystem2D::SetHightlightControlsVerticesLimit(uint32 verticesCount)
{
    highlightControlsVerticesLimit = verticesCount;
}

inline uint32 RenderSystem2D::GetVertexLayoutId(uint32 texCoordStreamCount)
{
    return vertexLayouts2d[texCoordStreamCount];
}
inline uint32 RenderSystem2D::GetVBOStride(uint32 texCoordStreamCount)
{
    return VBO_STRIDE[texCoordStreamCount];
}

inline float32 RenderSystem2D::AlignToX(float32 value)
{
    return std::floor(value / currentPhysicalToVirtualScale.x + 0.5f) * currentPhysicalToVirtualScale.x;
}

inline float32 RenderSystem2D::AlignToY(float32 value)
{
    return std::floor(value / currentPhysicalToVirtualScale.y + 0.5f) * currentPhysicalToVirtualScale.y;
}

inline Vector2 RenderSystem2D::GetAlignedVertex(const Vector2& vertex)
{
    return Vector2(AlignToX(vertex.x), AlignToY(vertex.y));
}

} // ns
