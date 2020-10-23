#include "Classes/Qt/DockLODEditor/DistanceSlider.h"

#include <REPlatform/DataNodes/Settings/RESettings.h>
#include <REPlatform/Scene/Systems/EditorLODSystem.h>

#include <TArc/Core/Deprecated.h>
#include <TArc/Utils/Utils.h>

#include <Debug/DVAssert.h>
#include <Functional/Function.h>
#include <Logger/Logger.h>
#include <Scene3D/Entity.h>
#include <Scene3D/Lod/LodComponent.h>
#include <Utils/StringFormat.h>

#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QSplitter>

namespace DistanceSliderDetail
{
const DAVA::int32 MIN_WIDGET_WIDTH = 1;
const DAVA::uint32 MAX_FRAMES_COUNT = DAVA::LodComponent::MAX_LOD_LAYERS + 1; // for layer at the end of distances

DAVA::int32 RoundFloat32(DAVA::float32 value)
{
    return static_cast<DAVA::int32>(DAVA::Round(value));
}

DAVA::int32 GetAvailableWidth(QSplitter* splitter, DAVA::uint32 visibleFramesCount)
{
    DAVA::int32 availableSplitterWidth = splitter->geometry().width();
    if (visibleFramesCount > 0)
    {
        availableSplitterWidth -= splitter->handleWidth() * (visibleFramesCount - 1);
    }

    return availableSplitterWidth;
}
}

DistanceSlider::DistanceSlider(QWidget* parent /*= 0*/)
    : QWidget(parent)
    , realDistances(DAVA::LodComponent::MAX_LOD_LAYERS, 0.0f)
    , multiple(DAVA::LodComponent::MAX_LOD_LAYERS, false)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setObjectName(QString::fromUtf8("layout"));

    splitter = new QSplitter(this);
    splitter->setObjectName(QString::fromUtf8("splitter"));
    splitter->setGeometry(geometry());
    splitter->setOrientation(Qt::Horizontal);
    splitter->setMinimumHeight(20);
    splitter->setChildrenCollapsible(false);
    splitter->installEventFilter(this);

    layout->addWidget(splitter);
    layout->setSpacing(0);
    layout->setMargin(0);
    setLayout(layout);

    InitFrames();

    connect(splitter, &QSplitter::splitterMoved, this, &DistanceSlider::SplitterMoved);
}

void DistanceSlider::InitFrames()
{
    DVASSERT(frames.empty());

    frames.reserve(DistanceSliderDetail::MAX_FRAMES_COUNT);
    for (DAVA::uint32 i = 0; i < DistanceSliderDetail::MAX_FRAMES_COUNT; ++i)
    {
        QFrame* frame = new QFrame(splitter);

        frame->setObjectName(QString::fromUtf8(DAVA::Format("frame_%d", i).c_str()));
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);

        frame->setAutoFillBackground(true);
        frame->setMinimumWidth(DistanceSliderDetail::MIN_WIDGET_WIDTH);

        splitter->addWidget(frame);

        frames.push_back(frame);
    }

    //install event filter for handlers
    DAVA::uint32 handlesCount = static_cast<DAVA::uint32>(splitterHandles.size());
    for (DAVA::uint32 i = handlesCount; i < DistanceSliderDetail::MAX_FRAMES_COUNT; i++)
    {
        QObject* obj = splitter->handle(static_cast<int>(i));
        obj->installEventFilter(this);
        splitterHandles.push_back(obj);
    }
}

void DistanceSlider::SetLayersCount(DAVA::uint32 count)
{
    if (layersCount != count)
    {
        layersCount = count;
        ColorizeUI();
    }
}

const DAVA::Vector<DAVA::float32>& DistanceSlider::GetDistances() const
{
    return realDistances;
}

void DistanceSlider::SetDistances(const DAVA::Vector<DAVA::float32>& distances_, const DAVA::Vector<bool>& multiple_)
{
    fitModeEnabled = DAVA::EditorLODSystem::IsFitModeEnabled(distances_);
    ApplyDistances(distances_);
    ApplyMultiple(multiple_);

    ColorizeUI();
}

void DistanceSlider::ApplyDistances(const DAVA::Vector<DAVA::float32>& distances_)
{
    DVASSERT(distances_.size() == DAVA::LodComponent::MAX_LOD_LAYERS);

    if (distances_ == realDistances)
    {
        return;
    }

    notInfDistancesCount = 0;
    for (DAVA::uint32 layer = 0, count = static_cast<DAVA::uint32>(realDistances.size()); layer < count; ++layer)
    {
        if (fabs(distances_[layer] - DAVA::EditorLODSystem::LOD_DISTANCE_INFINITY) < DAVA::EPSILON)
        {
            realDistances[layer] = distances_[layer];
        }
        else
        {
            ++notInfDistancesCount;
            realDistances[layer] = DAVA::Round(distances_[layer]);
        }
    }

    BuildUI();
}

void DistanceSlider::ApplyMultiple(const DAVA::Vector<bool>& multiple_)
{
    DVASSERT(multiple_.size() == DAVA::LodComponent::MAX_LOD_LAYERS);

    if (multiple != multiple_)
    {
        multiple = multiple_;
    }
}

void DistanceSlider::BuildUI()
{
    const DAVA::uint32 visibleFramesCount = (fitModeEnabled) ? notInfDistancesCount : notInfDistancesCount + 1;
    const DAVA::int32 availableSplitterWidth = DistanceSliderDetail::GetAvailableWidth(splitter, visibleFramesCount);
    const DAVA::float32 scaleSize = GetScaleSize();
    const DAVA::float32 widthCoef = availableSplitterWidth / scaleSize;

    QList<int> sizes;
    handlePositions.clear();

    DAVA::int32 lastLayerWidth = 0;
    DAVA::int32 handlePos = 0;
    const DAVA::int32 handleWidth = splitter->handleWidth();

    for (DAVA::uint32 i = 0; i < notInfDistancesCount; ++i)
    {
        DAVA::int32 prevWidth = lastLayerWidth;
        lastLayerWidth = DistanceSliderDetail::RoundFloat32(realDistances[i] * widthCoef);
        sizes.push_back(lastLayerWidth - prevWidth);

        handlePos += lastLayerWidth;
        handlePositions.push_back(handlePos);
        handlePos += handleWidth;
    }

    setToolTip("");
    if (!fitModeEnabled)
    {
        sizes.push_back(availableSplitterWidth - lastLayerWidth);
    }
    else if (visibleFramesCount == 0)
    {
        setToolTip("all distances are infinity");
    }

    QSignalBlocker guard(splitter);
    splitter->setSizes(sizes);
}

void DistanceSlider::ColorizeUI()
{
    DAVA::GeneralSettings* settings = DAVA::Deprecated::GetDataNode<DAVA::GeneralSettings>();
    DAVA::Vector<DAVA::Color> colors =
    {
      settings->lodEditorLodColor0,
      settings->lodEditorLodColor1,
      settings->lodEditorLodColor2,
      settings->lodEditorLodColor3
    };
    DVASSERT(colors.size() == DAVA::LodComponent::MAX_LOD_LAYERS);

    const QColor inactiveColor = DAVA::ColorToQColor(settings->inactiveColor);

    const DAVA::uint32 visibleFramesCount = (fitModeEnabled) ? notInfDistancesCount : notInfDistancesCount + 1;
    DAVA::uint32 coloredCount = DAVA::Min(layersCount, notInfDistancesCount);
    for (DAVA::uint32 i = 0; i < DistanceSliderDetail::MAX_FRAMES_COUNT; ++i)
    {
        QPalette pallete = frames[i]->palette();
        if (i < coloredCount)
        {
            if (multiple[i])
            {
                pallete.setColor(QPalette::Background, DAVA::ColorToQColor(DAVA::MakeGrayScale(colors[i])));
            }
            else
            {
                pallete.setColor(QPalette::Background, DAVA::ColorToQColor(colors[i]));
            }
        }
        else
        {
            pallete.setColor(QPalette::Background, inactiveColor);
        }
        frames[i]->setPalette(pallete);
        frames[i]->setVisible(i < visibleFramesCount);
    }
}

void DistanceSlider::SplitterMoved(int pos, int index)
{
    DVASSERT(index > 0);

    const DAVA::uint32 visibleFramesCount = (fitModeEnabled) ? notInfDistancesCount : notInfDistancesCount + 1;
    const DAVA::int32 availableSplitterWidth = DistanceSliderDetail::GetAvailableWidth(splitter, visibleFramesCount);

    const DAVA::float32 scaleSize = GetScaleSize();
    const DAVA::float32 widthCoef = scaleSize / availableSplitterWidth;

    QList<int> sizes = splitter->sizes();
    DVASSERT(static_cast<DAVA::int32>(notInfDistancesCount) < sizes.size());

    QList<int> newHandlePositions;
    const DAVA::int32 handleWidth = splitter->handleWidth();
    DAVA::int32 handlePos = 0;
    for (DAVA::int32 i = 0; i < sizes.size(); ++i)
    {
        handlePos += sizes.at(i);
        newHandlePositions.push_back(handlePos);
        handlePos += handleWidth;
    }

    DVASSERT(handlePositions.size() >= DAVA::int32(notInfDistancesCount));
    DVASSERT(newHandlePositions.size() >= DAVA::int32(notInfDistancesCount));
    for (DAVA::int32 i = 0; i < DAVA::int32(notInfDistancesCount); ++i)
    {
        if (newHandlePositions[i] != handlePositions[i])
        {
            realDistances[i] = DistanceSliderDetail::RoundFloat32(newHandlePositions[i] * widthCoef);
        }
    }

    handlePositions = newHandlePositions;

    emit DistanceHandleMoved();
}

DAVA::float32 DistanceSlider::GetScaleSize() const
{
    DAVA::float32 minVisualValue = DAVA::LodComponent::MIN_LOD_DISTANCE;
    DAVA::float32 maxVisualValue = DAVA::LodComponent::MAX_LOD_DISTANCE;

    if (fitModeEnabled && notInfDistancesCount > 0)
    {
        maxVisualValue = realDistances[notInfDistancesCount - 1];
    }

    return maxVisualValue - minVisualValue;
}

bool DistanceSlider::eventFilter(QObject* obj, QEvent* e)
{
    bool retValue = QWidget::eventFilter(obj, e);
    if (e->type() == QEvent::MouseButtonRelease)
    {
        if (std::find(splitterHandles.begin(), splitterHandles.end(), obj) != splitterHandles.end())
        {
            emit DistanceHandleReleased();
        }
    }
    if (e->type() == QEvent::Resize && obj == splitter)
    {
        BuildUI();
    }

    return retValue;
}
