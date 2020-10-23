#include "RulerToolSystem.h"
#include "CollisionSystem.h"
#include "ModifSystem.h"
#include "LandscapeEditorDrawSystem.h"
#include "../SceneEditor2.h"
#include "LandscapeEditorDrawSystem/LandscapeProxy.h"
#include "LandscapeEditorDrawSystem/RulerToolProxy.h"
#include "LandscapeEditorDrawSystem/HeightmapProxy.h"
#include "../Qt/Main/QtUtils.h"
#include "../SceneSignals.h"

#include "Classes/Selection/Selection.h"

RulerToolSystem::RulerToolSystem(DAVA::Scene* scene)
    : LandscapeEditorSystem(scene, DefaultCursorPath())
    , curToolSize(0)
    , previewEnabled(true)
{
}

RulerToolSystem::~RulerToolSystem()
{
}

LandscapeEditorDrawSystem::eErrorType RulerToolSystem::EnableLandscapeEditing()
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

    LandscapeEditorDrawSystem::eErrorType enableCustomDrawError = drawSystem->EnableCustomDraw();
    if (enableCustomDrawError != LandscapeEditorDrawSystem::LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS)
    {
        return enableCustomDrawError;
    }

    bool inputLocked = AcquireInputLock(GetScene());
    Selection::Lock();
    DVASSERT(inputLocked);

    DAVA::Texture* rulerToolTexture = drawSystem->GetRulerToolProxy()->GetTexture();
    drawSystem->GetLandscapeProxy()->SetToolTexture(rulerToolTexture, false);
    landscapeSize = drawSystem->GetHeightmapProxy()->Size();

    previewLength = -1.f;
    previewEnabled = true;

    ClearInternal();
    DrawPoints();

    SendUpdatedLength();

    enabled = true;
    return LandscapeEditorDrawSystem::LANDSCAPE_EDITOR_SYSTEM_NO_ERRORS;
}

bool RulerToolSystem::DisableLandscapeEdititing()
{
    if (!enabled)
    {
        return true;
    }

    ReleaseInputLock(GetScene());
    Selection::Unlock();
    drawSystem->DisableCustomDraw();

    drawSystem->GetLandscapeProxy()->SetToolTexture(nullptr, false);

    ClearInternal();
    previewLength = -1.f;
    SendUpdatedLength();

    enabled = false;
    return !enabled;
}

void RulerToolSystem::PrepareForRemove()
{
    ClearInternal();
}

void RulerToolSystem::Process(DAVA::float32 timeElapsed)
{
    if (!IsLandscapeEditingEnabled())
    {
        return;
    }
}

bool RulerToolSystem::Input(DAVA::UIEvent* event)
{
    if (!IsLandscapeEditingEnabled())
    {
        return false;
    }

    UpdateCursorPosition();

    DAVA::Vector3 point3;
    collisionSystem->LandRayTestFromCamera(point3);
    DAVA::Vector2 point(point3.x, point3.y);

    switch (event->phase)
    {
    case DAVA::UIEvent::Phase::KEY_DOWN:
    case DAVA::UIEvent::Phase::KEY_DOWN_REPEAT:
        if (DAVA::eInputElements::KB_BACKSPACE == event->key)
        {
            RemoveLastPoint();
            previewEnabled = true;
            CalcPreviewPoint(point, true);
        }
        else if (DAVA::eInputElements::KB_ESCAPE == event->key)
        {
            previewEnabled = false;
        }
        DrawPoints();
        break;

    case DAVA::UIEvent::Phase::MOVE:
        if (previewEnabled)
        {
            CalcPreviewPoint(point);
            DrawPoints();
        }
        break;

    case DAVA::UIEvent::Phase::ENDED:
        if (event->mouseButton == DAVA::eMouseButtons::LEFT && isIntersectsLandscape)
        {
            if (IsKeyModificatorPressed(DAVA::eInputElements::KB_LSHIFT))
            {
                SetStartPoint(point);
            }
            else
            {
                if (previewEnabled)
                {
                    AddPoint(point);
                }
            }

            previewEnabled = true;
            CalcPreviewPoint(point);
            DrawPoints();
        }
        break;

    default:
        break;
    }
    return false;
}

void RulerToolSystem::SetStartPoint(const DAVA::Vector2& point)
{
    ClearInternal();

    previewPoint = point;
    linePoints.push_back(point);
    lengths.push_back(0.f);
    SendUpdatedLength();
}

void RulerToolSystem::AddPoint(const DAVA::Vector2& point)
{
    if (0 < linePoints.size())
    {
        DAVA::Vector2 prevPoint = *(linePoints.rbegin());
        DAVA::float32 l = lengths.back();
        l += GetLength(prevPoint, point);

        linePoints.push_back(point);
        lengths.push_back(l);

        SendUpdatedLength();
    }
}

void RulerToolSystem::RemoveLastPoint()
{
    //remove points except start point
    if (linePoints.size() > 1)
    {
        DAVA::List<DAVA::Vector2>::iterator pointsIter = linePoints.end();
        --pointsIter;
        linePoints.erase(pointsIter);

        DAVA::List<DAVA::float32>::iterator lengthsIter = lengths.end();
        --lengthsIter;
        lengths.erase(lengthsIter);

        SendUpdatedLength();
    }
}

void RulerToolSystem::CalcPreviewPoint(const DAVA::Vector2& point, bool force)
{
    if (!previewEnabled)
    {
        return;
    }

    if ((isIntersectsLandscape && linePoints.size() > 0) && (force || previewPoint != point))
    {
        DAVA::Vector2 lastPoint = linePoints.back();
        DAVA::float32 previewLen = GetLength(lastPoint, point);

        previewPoint = point;
        previewLength = lengths.back() + previewLen;
    }
    else if (!isIntersectsLandscape)
    {
        previewLength = -1.f;
    }
    SendUpdatedLength();
}

DAVA::float32 RulerToolSystem::GetLength(const DAVA::Vector2& startPoint, const DAVA::Vector2& endPoint)
{
    DAVA::float32 lineSize = 0.f;

    DAVA::Vector3 prevPoint = DAVA::Vector3(startPoint);
    DAVA::Vector3 prevLandscapePoint = drawSystem->GetLandscapeProxy()->PlacePoint(prevPoint); //

    for (DAVA::int32 i = 1; i <= APPROXIMATION_COUNT; ++i)
    {
        DAVA::Vector3 point = DAVA::Vector3(startPoint + (endPoint - startPoint) * i / static_cast<DAVA::float32>(APPROXIMATION_COUNT));
        DAVA::Vector3 landscapePoint = drawSystem->GetLandscapeProxy()->PlacePoint(point); //

        lineSize += (landscapePoint - prevLandscapePoint).Length();

        prevPoint = point;
        prevLandscapePoint = landscapePoint;
    }

    return lineSize;
}

void RulerToolSystem::DrawPoints()
{
    if (!drawSystem->GetRulerToolProxy())
    {
        return;
    }

    DAVA::Texture* targetTexture = drawSystem->GetRulerToolProxy()->GetTexture();

    DAVA::RenderSystem2D::RenderTargetPassDescriptor desc;
    desc.colorAttachment = targetTexture->handle;
    desc.depthAttachment = targetTexture->handleDepthStencil;
    desc.width = targetTexture->GetWidth();
    desc.height = targetTexture->GetHeight();
    desc.transformVirtualToPhysical = false;
    DAVA::RenderSystem2D::Instance()->BeginRenderTargetPass(desc);

    DAVA::Vector<DAVA::Vector2> points;
    points.reserve(linePoints.size() + 1);
    std::copy(linePoints.begin(), linePoints.end(), std::back_inserter(points));

    if (previewEnabled && isIntersectsLandscape)
    {
        points.push_back(previewPoint);
    }

    const DAVA::uint32 pointsCount = static_cast<DAVA::uint32>(points.size());
    if (pointsCount > 1)
    {
        for (DAVA::uint32 i = 0; i < pointsCount; ++i)
        {
            points[i] = MirrorPoint(points[i]);
        }

        DAVA::Color red(1.0f, 0.0f, 0.0f, 1.0f);
        DAVA::Color blue(0.f, 0.f, 1.f, 1.f);

        const DAVA::AABBox3& boundingBox = drawSystem->GetLandscapeProxy()->GetLandscapeBoundingBox();
        const DAVA::Vector3 landSize = boundingBox.max - boundingBox.min;
        DAVA::Vector2 offsetPoint = DAVA::Vector2(boundingBox.min.x, boundingBox.min.y);

        DAVA::float32 koef = static_cast<DAVA::float32>(targetTexture->GetWidth()) / landSize.x;

        DAVA::Vector2 startPoint = points[0];
        for (DAVA::uint32 i = 1; i < pointsCount; ++i)
        {
            DAVA::Vector2 endPoint = points[i];

            DAVA::Vector2 startPosition = (startPoint - offsetPoint) * koef;
            DAVA::Vector2 endPosition = (endPoint - offsetPoint) * koef;

            if (previewEnabled && isIntersectsLandscape && i == (points.size() - 1))
            {
                DAVA::RenderSystem2D::Instance()->DrawLine(startPosition, endPosition, blue);
            }
            else
            {
                DAVA::RenderSystem2D::Instance()->DrawLine(startPosition, endPosition, red);
            }

            startPoint = endPoint;
        }
    }

    DAVA::RenderSystem2D::Instance()->EndRenderTargetPass();
}

void RulerToolSystem::ClearInternal()
{
    linePoints.clear();
    lengths.clear();
}

void RulerToolSystem::DisablePreview()
{
    previewEnabled = false;
    previewLength = -1.f;

    SendUpdatedLength();
}

void RulerToolSystem::SendUpdatedLength()
{
    DAVA::float32 length = GetLength();
    DAVA::float32 previewLength = GetPreviewLength();

    SceneSignals::Instance()->EmitRulerToolLengthChanged(dynamic_cast<SceneEditor2*>(GetScene()),
                                                         length, previewLength);
}

DAVA::float32 RulerToolSystem::GetLength()
{
    DAVA::float32 length = -1.f;
    if (lengths.size() > 0)
    {
        length = lengths.back();
    }

    return length;
}

DAVA::float32 RulerToolSystem::GetPreviewLength()
{
    DAVA::float32 previewLength = -1.f;
    if (previewEnabled)
    {
        previewLength = this->previewLength;
    }

    return previewLength;
}

DAVA::Vector2 RulerToolSystem::MirrorPoint(const DAVA::Vector2& point) const
{
    const DAVA::AABBox3& boundingBox = drawSystem->GetLandscapeProxy()->GetLandscapeBoundingBox();

    DAVA::Vector2 newPoint = point;
    newPoint.y = (boundingBox.max.y - point.y) + boundingBox.min.y;

    return newPoint;
}
