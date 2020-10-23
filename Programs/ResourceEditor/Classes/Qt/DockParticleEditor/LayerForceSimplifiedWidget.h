#pragma once

#include "BaseParticleEditorContentWidget.h"
#include <REPlatform/Scene/SceneEditor2.h>
#include <DAVAEngine.h>

#include <QWidget>

class TimeLineWidget;
class QVBoxLayout;

class LayerForceSimplifiedWidget : public BaseParticleEditorContentWidget
{
    Q_OBJECT

public:
    explicit LayerForceSimplifiedWidget(QWidget* parent = 0);
    ~LayerForceSimplifiedWidget();

    void Init(DAVA::SceneEditor2* scene, DAVA::ParticleLayer* layer, DAVA::uint32 forceIndex, bool updateMinimized);
    DAVA::ParticleLayer* GetLayer() const
    {
        return layer;
    };
    DAVA::int32 GetForceIndex() const
    {
        return forceIndex;
    };

    void Update();

    virtual void StoreVisualState(DAVA::KeyedArchive* visualStateProps);
    virtual void RestoreVisualState(DAVA::KeyedArchive* visualStateProps);

signals:
    void ValueChanged();

protected slots:
    void OnValueChanged();

protected:
    void InitWidget(QWidget* widget);

private:
    QVBoxLayout* mainBox;
    DAVA::ParticleLayer* layer;
    DAVA::int32 forceIndex;

    TimeLineWidget* forceTimeLine;
    TimeLineWidget* forceOverLifeTimeLine;

    bool blockSignals;
};
