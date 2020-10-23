#include "NotPassableTerrainProxy.h"

static const DAVA::int32 GRID_QUAD_SIZE = 65;

NotPassableTerrainProxy::NotPassableTerrainProxy(DAVA::int32 heightmapSize)
    : enabled(false)
{
    LoadColorsArray();

    notPassableAngleTan = static_cast<DAVA::float32>(tan(DAVA::DegToRad(static_cast<DAVA::float32>(NOT_PASSABLE_ANGLE))));
    notPassableTexture = DAVA::Texture::CreateFBO(2048, 2048, DAVA::FORMAT_RGBA8888);
    notPassableTexture->SetMinMagFilter(rhi::TextureFilter::TEXFILTER_LINEAR, rhi::TextureFilter::TEXFILTER_LINEAR, rhi::TextureMipFilter::TEXMIPFILTER_NONE);

    rhi::Viewport viewport;
    viewport.width = viewport.height = 2048;
    DAVA::RenderHelper::CreateClearPass(notPassableTexture->handle, rhi::HTexture(), DAVA::PRIORITY_SERVICE_2D + 1, DAVA::Color::Clear, viewport);

    DAVA::int32 quadLineSize = static_cast<DAVA::int32>(ceilf(static_cast<DAVA::float32>(heightmapSize) / GRID_QUAD_SIZE));
    DAVA::int32 buffersCount = quadLineSize * quadLineSize;
    gridBuffers.resize(buffersCount);

    rhi::VertexBuffer::Descriptor desc;
    desc.size = GRID_QUAD_SIZE * GRID_QUAD_SIZE * 4 * sizeof(DAVA::float32) * 4;
    desc.needRestore = false;

    for (DAVA::int32 i = 0; i < buffersCount; ++i)
        gridBuffers[i] = rhi::CreateVertexBuffer(desc);
}

NotPassableTerrainProxy::~NotPassableTerrainProxy()
{
    SafeRelease(notPassableTexture);
    for (const rhi::HVertexBuffer& quadBuffer : gridBuffers)
        rhi::DeleteVertexBuffer(quadBuffer);
}

void NotPassableTerrainProxy::LoadColorsArray()
{
    DAVA::YamlParser* parser = DAVA::YamlParser::Create("~res:/ResourceEditor/Configs/LandscapeAngle.yaml");

    if (parser != 0)
    {
        DAVA::YamlNode* rootNode = parser->GetRootNode();
        DAVA::int32 anglesCount = rootNode->GetCount();

        angleColor.reserve(anglesCount);
        for (DAVA::int32 i = 0; i < anglesCount; ++i)
        {
            const DAVA::YamlNode* node = rootNode->Get(i);
            if (!node || node->GetCount() != 3)
            {
                continue;
            }

            DAVA::float32 angle1 = node->Get(0)->AsFloat();
            DAVA::float32 angle2 = node->Get(1)->AsFloat();

            angle1 = DAVA::Min(angle1, 89.f);
            angle2 = DAVA::Min(angle2, 89.f);

            DAVA::float32 tangentMin = tan(DAVA::DegToRad(angle1));
            DAVA::float32 tangentMax = tan(DAVA::DegToRad(angle2));

            const DAVA::YamlNode* colorNode = node->Get(2);
            if (!colorNode || colorNode->GetCount() != 4)
            {
                continue;
            }

            DAVA::Color color(colorNode->Get(0)->AsFloat() / 255.f,
                              colorNode->Get(1)->AsFloat() / 255.f,
                              colorNode->Get(2)->AsFloat() / 255.f,
                              colorNode->Get(3)->AsFloat() / 255.f);

            angleColor.push_back(TerrainColor(DAVA::Vector2(tangentMin, tangentMax), color));
        }
    }

    DAVA::SafeRelease(parser);
}

bool NotPassableTerrainProxy::PickColor(DAVA::float32 tan, DAVA::Color& color) const
{
    for (DAVA::uint32 i = 0; i < angleColor.size(); ++i)
    {
        if (tan >= angleColor[i].angleRange.x && tan < angleColor[i].angleRange.y)
        {
            color = angleColor[i].color;
            return true;
        }
    }
    return false;
}

void NotPassableTerrainProxy::SetEnabled(bool enabled_)
{
    enabled = enabled_;
}

bool NotPassableTerrainProxy::IsEnabled() const
{
    return enabled;
}

DAVA::Texture* NotPassableTerrainProxy::GetTexture()
{
    return notPassableTexture;
}

void NotPassableTerrainProxy::UpdateTexture(DAVA::Heightmap* heightmap, const DAVA::AABBox3& landscapeBoundingBox, const DAVA::Rect2i& forRect)
{
    const DAVA::Vector3 landSize = landscapeBoundingBox.max - landscapeBoundingBox.min;

    const DAVA::float32 angleCellDistance = landSize.x / static_cast<DAVA::float32>(heightmap->Size());
    const DAVA::float32 angleHeightDelta = landSize.z / static_cast<DAVA::float32>(DAVA::Heightmap::MAX_VALUE - 1);
    const DAVA::float32 tanCoef = angleHeightDelta / angleCellDistance;

    const DAVA::int32 heightmapSize = heightmap->Size();

    const DAVA::float32 targetWidth = static_cast<DAVA::float32>(notPassableTexture->GetWidth());
    const DAVA::float32 dx = targetWidth / static_cast<DAVA::float32>(heightmapSize);

    ///////////////////////////////

    DAVA::Size2f textureSize(DAVA::float32(notPassableTexture->GetWidth()), DAVA::float32(notPassableTexture->GetHeight()));

    DAVA::Matrix4 projMatrix;
    if (!rhi::DeviceCaps().isUpperLeftRTOrigin)
    {
        projMatrix.BuildOrtho(0.0f, textureSize.dx, 0.0f, textureSize.dy, -1.0f, 1.0f, rhi::DeviceCaps().isZeroBaseClipRange);
    }
    else
    {
        projMatrix.BuildOrtho(0.0f, textureSize.dx, textureSize.dy, 0.0f, -1.0f, 1.0f, rhi::DeviceCaps().isZeroBaseClipRange);
    }

    DAVA::Renderer::GetDynamicBindings().SetDynamicParam(DAVA::DynamicBindings::PARAM_WORLD, &DAVA::Matrix4::IDENTITY, reinterpret_cast<DAVA::pointer_size>(&DAVA::Matrix4::IDENTITY));
    DAVA::Renderer::GetDynamicBindings().SetDynamicParam(DAVA::DynamicBindings::PARAM_VIEW, &DAVA::Matrix4::IDENTITY, reinterpret_cast<DAVA::pointer_size>(&DAVA::Matrix4::IDENTITY));
    DAVA::Renderer::GetDynamicBindings().SetDynamicParam(DAVA::DynamicBindings::PARAM_PROJ, &projMatrix, DAVA::DynamicBindings::UPDATE_SEMANTIC_ALWAYS);

    rhi::Packet gridQuadPacket;
    DAVA::RenderSystem2D::DEFAULT_2D_COLOR_MATERIAL->BindParams(gridQuadPacket);

    rhi::VertexLayout layout;
    layout.AddElement(rhi::VS_POSITION, 0, rhi::VDT_FLOAT, 3);
    layout.AddElement(rhi::VS_COLOR, 0, rhi::VDT_UINT8N, 4);

    gridQuadPacket.vertexStreamCount = 1;
    gridQuadPacket.primitiveType = rhi::PRIMITIVE_LINELIST;
    gridQuadPacket.vertexLayoutUID = rhi::VertexLayout::UniqueId(layout);

    ///////////////////////////////

    rhi::RenderPassConfig passConfig;
    passConfig.colorBuffer[0].texture = notPassableTexture->handle;
    passConfig.colorBuffer[0].loadAction = rhi::LOADACTION_NONE;
    passConfig.colorBuffer[0].storeAction = rhi::STOREACTION_STORE;
    passConfig.priority = DAVA::PRIORITY_SERVICE_2D;
    passConfig.viewport.width = notPassableTexture->GetWidth();
    passConfig.viewport.height = notPassableTexture->GetHeight();

    rhi::HPacketList packetListHandle;
    rhi::HRenderPass passTargetHandle = rhi::AllocateRenderPass(passConfig, 1, &packetListHandle);

    rhi::BeginRenderPass(passTargetHandle);
    rhi::BeginPacketList(packetListHandle);

    ///////////////////////////////

    DAVA::int32 bufferIndex = 0;
    DAVA::float32* tempBuffer = new DAVA::float32[GRID_QUAD_SIZE * GRID_QUAD_SIZE * 4 * 4];
    for (DAVA::int32 yRect = 0; yRect < heightmapSize; yRect += GRID_QUAD_SIZE)
    {
        for (DAVA::int32 xRect = 0; xRect < heightmapSize; xRect += GRID_QUAD_SIZE)
        {
            if (DAVA::Rect2i(xRect, yRect, GRID_QUAD_SIZE, GRID_QUAD_SIZE).RectIntersects(forRect))
            {
                DAVA::float32* bufferPtr = tempBuffer;
                DAVA::int32 primitiveCount = 0;

                for (DAVA::int32 y = yRect; (y < yRect + GRID_QUAD_SIZE) && y < heightmapSize; ++y)
                {
                    const DAVA::float32 ydx = (heightmapSize - y) * dx;

                    for (DAVA::int32 x = xRect; (x < xRect + GRID_QUAD_SIZE) && x < heightmapSize; ++x)
                    {
                        const DAVA::uint16 currentPoint = heightmap->GetHeightClamp(x, y);
                        const DAVA::uint16 rightPoint = heightmap->GetHeightClamp(x + 1, y);
                        const DAVA::uint16 bottomPoint = heightmap->GetHeightClamp(x, y + 1);

                        const DAVA::uint16 deltaRight = static_cast<DAVA::uint16>(abs(static_cast<DAVA::int32>(currentPoint) - static_cast<DAVA::int32>(rightPoint)));
                        const DAVA::uint16 deltaBottom = static_cast<DAVA::uint16>(abs(static_cast<DAVA::int32>(currentPoint) - static_cast<DAVA::int32>(bottomPoint)));

                        const DAVA::float32 tanRight = static_cast<DAVA::float32>(deltaRight) * tanCoef;
                        const DAVA::float32 tanBottom = static_cast<DAVA::float32>(deltaBottom) * tanCoef;

                        const DAVA::float32 xdx = x * dx;

                        DAVA::Color color(0.f, 0.f, 0.f, 0.f);

                        PickColor(tanRight, color);

                        {
                            *(reinterpret_cast<DAVA::Vector3*>(bufferPtr)) = DAVA::Vector3(xdx, ydx, 0.f);
                            bufferPtr += 3;
                            *(reinterpret_cast<DAVA::uint32*>(bufferPtr)) = rhi::NativeColorRGBA(color.r, color.g, color.b, color.a);
                            ++bufferPtr;
                            *(reinterpret_cast<DAVA::Vector3*>(bufferPtr)) = DAVA::Vector3((xdx + dx), ydx, 0.f);
                            bufferPtr += 3;
                            *(reinterpret_cast<DAVA::uint32*>(bufferPtr)) = rhi::NativeColorRGBA(color.r, color.g, color.b, color.a);
                            ++bufferPtr;
                        }

                        PickColor(tanBottom, color);

                        {
                            *(reinterpret_cast<DAVA::Vector3*>(bufferPtr)) = DAVA::Vector3(xdx, ydx, 0.f);
                            bufferPtr += 3;
                            *(reinterpret_cast<DAVA::uint32*>(bufferPtr)) = rhi::NativeColorRGBA(color.r, color.g, color.b, color.a);
                            ++bufferPtr;
                            *(reinterpret_cast<DAVA::Vector3*>(bufferPtr)) = DAVA::Vector3(xdx, (ydx - dx), 0.f);
                            bufferPtr += 3;
                            *(reinterpret_cast<DAVA::uint32*>(bufferPtr)) = rhi::NativeColorRGBA(color.r, color.g, color.b, color.a);
                            ++bufferPtr;
                        }

                        primitiveCount += 2;
                    }
                }

                rhi::UpdateVertexBuffer(gridBuffers[bufferIndex], tempBuffer, 0, primitiveCount * 4 * 2 * sizeof(DAVA::float32));

                gridQuadPacket.vertexStream[0] = gridBuffers[bufferIndex];
                gridQuadPacket.primitiveCount = primitiveCount;
                gridQuadPacket.vertexCount = gridQuadPacket.primitiveCount * 2;

                rhi::AddPacket(packetListHandle, gridQuadPacket);
            }

            ++bufferIndex;
        }
    }

    DAVA::SafeDeleteArray(tempBuffer);

    ///////////////////////////////

    rhi::EndPacketList(packetListHandle);
    rhi::EndRenderPass(passTargetHandle);
}
