#ifndef __PARTICLE_TIME_LINE_COLUMNS__H__
#define __PARTICLE_TIME_LINE_COLUMNS__H__

#include "DAVAEngine.h"

#include <QWidget>
#include "ParticleTimeLineWidget.h"

////////////////////////////////////////////////////////////////////////////////////
// A specific class to display the per-layer particles info.
const DAVA::int32 EXTRA_INFO_DEFAULT_COLUMN_WIDTH = 60;
const DAVA::int32 EXTRA_INFO_LEFT_PADDING = 3;

// Sizes for specific columns.
const DAVA::int32 EXTRA_INFO_COUNT_COLUMN_WIDTH = 40;
const DAVA::int32 EXTRA_INFO_MAX_COUNT_COLUMN_WIDTH = 52;
const DAVA::int32 EXTRA_INFO_AREA_COLUMN_WIDTH = 50;

class ParticlesExtraInfoColumn : public QWidget
{
    Q_OBJECT

public:
    explicit ParticlesExtraInfoColumn(const ParticleTimeLineWidget* timeLineWidget,
                                      QWidget* parent = 0);

    // This method is called when the list of layers is changed.
    virtual void OnLayersListChanged(){};

    // This method returns the preferred column's width.
    virtual DAVA::int32 GetColumnWidth()
    {
        return EXTRA_INFO_DEFAULT_COLUMN_WIDTH;
    };

    // This method is called when the appropriate particle effect is started/stopped/restarted.
    virtual void Reset(){};

protected:
    virtual void paintEvent(QPaintEvent*);

    // These methods are to be overriden for derived classes.
    // Get an extra information to be displayed near the line.
    virtual QString GetExtraInfoForLayerLine(DAVA::ParticleEffectComponent* effect, const ParticleTimeLineWidget::LINE& line)
    {
        return QString();
    };

    // Get the extra info for the header/footer.
    virtual QString GetExtraInfoHeader()
    {
        return QString();
    };
    virtual QString GetExtraInfoFooter()
    {
        return QString();
    };

    // In case some information should be accumulated during the loop,
    // these methods are called just before the loop and just after it.
    virtual void OnBeforeGetExtraInfoLoop(){};
    virtual void OnAfterGetExtraInfoLoop(){};

    // The timeline widget being used.
    const ParticleTimeLineWidget* timeLineWidget;

    // Helper methods.
    QString FormatFloat(DAVA::float32 value);
};

// Base class for Cumulative Columns.
class ParticlesExtraInfoCumulativeColumn : public ParticlesExtraInfoColumn
{
    Q_OBJECT
public:
    explicit ParticlesExtraInfoCumulativeColumn(const ParticleTimeLineWidget* timeLineWidget,
                                                QWidget* parent = 0);
    virtual void OnLayersListChanged();

protected:
    // Add the value to the Cumulative Data.
    void UpdateCumulativeData(DAVA::ParticleLayer* layer, DAVA::float32 value);

    // Update the value in Cumulative Data if the new value is bigger than existing one.
    void UpdateCumulativeDataIfMaximum(DAVA::ParticleLayer* layer, DAVA::float32 value);

    // Cleanup all the Cumulative Data.
    void CleanupCumulativeData();

    DAVA::int32 totalParticlesCount;
    DAVA::float32 totalParticlesArea;
    DAVA::int32 totalUpdatesCount;

    DAVA::Map<DAVA::ParticleLayer*, DAVA::float32> cumulativeData;
};

// Particles Count information.
class ParticlesCountColumn : public ParticlesExtraInfoColumn
{
    Q_OBJECT
public:
    explicit ParticlesCountColumn(const ParticleTimeLineWidget* timeLineWidget,
                                  QWidget* parent = 0);

    virtual DAVA::int32 GetColumnWidth()
    {
        return EXTRA_INFO_COUNT_COLUMN_WIDTH;
    };

protected:
    virtual void OnBeforeGetExtraInfoLoop();
    virtual QString GetExtraInfoForLayerLine(DAVA::ParticleEffectComponent* effect, const ParticleTimeLineWidget::LINE& line);

    virtual QString GetExtraInfoHeader();
    virtual QString GetExtraInfoFooter();

private:
    DAVA::int32 totalParticlesCount;
};

class ParticlesAverageCountColumn : public ParticlesExtraInfoCumulativeColumn
{
    Q_OBJECT
public:
    explicit ParticlesAverageCountColumn(const ParticleTimeLineWidget* timeLineWidget,
                                         QWidget* parent = 0);

    virtual void Reset();

protected:
    virtual QString GetExtraInfoForLayerLine(DAVA::ParticleEffectComponent* effect, const ParticleTimeLineWidget::LINE& line);

    virtual QString GetExtraInfoHeader();
    virtual QString GetExtraInfoFooter();
};

class ParticlesMaxCountColumn : public ParticlesExtraInfoCumulativeColumn
{
    Q_OBJECT
public:
    explicit ParticlesMaxCountColumn(const ParticleTimeLineWidget* timeLineWidget,
                                     QWidget* parent = 0);

    virtual DAVA::int32 GetColumnWidth()
    {
        return EXTRA_INFO_MAX_COUNT_COLUMN_WIDTH;
    };
    virtual void OnLayersListChanged();
    virtual void Reset();

protected:
    virtual QString GetExtraInfoForLayerLine(DAVA::ParticleEffectComponent* effect, const ParticleTimeLineWidget::LINE& line);

    virtual void OnBeforeGetExtraInfoLoop();
    virtual void OnAfterGetExtraInfoLoop();

    virtual QString GetExtraInfoHeader();
    virtual QString GetExtraInfoFooter();

private:
    DAVA::int32 maxParticlesCount;
    DAVA::int32 totalParticlesCountOnThisLoop;
};

// Particles Area information.
class ParticlesAreaColumn : public ParticlesExtraInfoColumn
{
    Q_OBJECT
public:
    explicit ParticlesAreaColumn(const ParticleTimeLineWidget* timeLineWidget,
                                 QWidget* parent = 0);
    virtual DAVA::int32 GetColumnWidth()
    {
        return EXTRA_INFO_AREA_COLUMN_WIDTH;
    };

protected:
    virtual void OnBeforeGetExtraInfoLoop();
    virtual QString GetExtraInfoForLayerLine(DAVA::ParticleEffectComponent* effect, const ParticleTimeLineWidget::LINE& line);

    virtual QString GetExtraInfoHeader();
    virtual QString GetExtraInfoFooter();

private:
    DAVA::float32 totalParticlesArea;
};

class ParticlesAverageAreaColumn : public ParticlesExtraInfoCumulativeColumn
{
    Q_OBJECT
public:
    explicit ParticlesAverageAreaColumn(const ParticleTimeLineWidget* timeLineWidget,
                                        QWidget* parent = 0);
    virtual void Reset();

protected:
    virtual QString GetExtraInfoForLayerLine(DAVA::ParticleEffectComponent* effect, const ParticleTimeLineWidget::LINE& line);

    virtual QString GetExtraInfoHeader();
    virtual QString GetExtraInfoFooter();
};

class ParticlesMaxAreaColumn : public ParticlesExtraInfoCumulativeColumn
{
    Q_OBJECT
public:
    explicit ParticlesMaxAreaColumn(const ParticleTimeLineWidget* timeLineWidget,
                                    QWidget* parent = 0);
    virtual void OnLayersListChanged();
    virtual void Reset();

protected:
    virtual QString GetExtraInfoForLayerLine(DAVA::ParticleEffectComponent* effect, const ParticleTimeLineWidget::LINE& line);

    virtual void OnBeforeGetExtraInfoLoop();
    virtual void OnAfterGetExtraInfoLoop();

    virtual QString GetExtraInfoHeader();
    virtual QString GetExtraInfoFooter();

private:
    DAVA::float32 maxParticlesArea;
    DAVA::float32 totalParticlesAreaOnThisLoop;
};


#endif /* __PARTICLE_TIME_LINE_COLUMNS__H__ */
