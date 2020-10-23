#pragma once

#include "Base/BaseTypes.h"
#include "Scene3D/Lod/LodComponent.h"

#include <QWidget>

class QSplitter;
class QFrame;

class LazyUpdater;

class DistanceSlider final : public QWidget
{
    Q_OBJECT

public:
    DistanceSlider(QWidget* parent = 0);

    void SetLayersCount(DAVA::uint32 count);
    DAVA::uint32 GetLayersCount() const;

    void SetDistances(const DAVA::Vector<DAVA::float32>& distances, const DAVA::Vector<bool>& multiple);
    const DAVA::Vector<DAVA::float32>& GetDistances() const;

signals:
    void DistanceHandleMoved();
    void DistanceHandleReleased();

private slots:
    void SplitterMoved(int pos, int index);

private:
    bool eventFilter(QObject* obj, QEvent* e) override;

    DAVA::float32 GetScaleSize() const;

    void InitFrames();
    void BuildUI();
    void ColorizeUI();

    void ApplyDistances(const DAVA::Vector<DAVA::float32>& distances);
    void ApplyMultiple(const DAVA::Vector<bool>& multiple);

    QSplitter* splitter = nullptr;
    DAVA::Vector<QObject*> splitterHandles;

    DAVA::Vector<QFrame*> frames;
    DAVA::Vector<DAVA::float32> realDistances;
    DAVA::Vector<bool> multiple;

    QList<int> handlePositions;

    DAVA::uint32 layersCount = 0;
    DAVA::uint32 notInfDistancesCount = 0;

    bool fitModeEnabled = false;
};

inline DAVA::uint32 DistanceSlider::GetLayersCount() const
{
    return layersCount;
}
