#pragma once

#include "UI/Preview/Guides/IRulerListener.h"
#include "UI/Preview/Guides/Guide.h"
#include "Modules/CanvasModule/CanvasDataAdapter.h"

#include "Model/PackageHierarchy/PackageNode.h"

#include <TArc/DataProcessing/DataListener.h>
#include <TArc/DataProcessing/DataWrapper.h>
#include <TArc/Utils/QtDelayedExecutor.h>

#include <Base/Any.h>
#include <Math/Vector.h>
#include <Math/Color.h>

#include <QWidget>
#include <QMap>
#include <QObject>
#include <QPointer>

namespace DAVA
{
class ContextAccessor;
class FieldBinder;
}

class CanvasData;
class CentralWidgetData;
class DocumentData;

class QWidget;
class QPropertyAnimation;

//this class realize Behavior pattern to have different behaviors for vertical and for horizontal guides
class GuidesController : public QObject, public IRulerListener, DAVA::DataListener
{
    Q_OBJECT

public:
    GuidesController(DAVA::Vector2::eAxis orientation, DAVA::ContextAccessor* accessor, QWidget* container);

private:
    void OnDataChanged(const DAVA::DataWrapper& wrapper, const DAVA::Vector<DAVA::Any>& fields) override;

    void BindFields();
    void OnCanvasParametersChanged(const DAVA::Any&);

    //IRulerListener
    void OnMousePress(DAVA::float32 position) override;
    void OnMouseMove(DAVA::float32 position) override;
    void OnMouseRelease(DAVA::float32 position) override;
    void OnMouseLeave() override;
    QList<QAction*> GetActions(DAVA::float32 position, QObject* parent) override;

    //cursor and preview guide states
    //NO_DISPLAY: normal cursor and no preview guide
    //DISPLAY_PREVIEW: normal cursor and preview guide is visible
    //DISPLAY_DRAG: drag cursor and no preview guide
    //DISPLAY_REMOVE: cursor changed to RemoveCursor and no preview guide
    enum eDisplayState
    {
        NO_DISPLAY,
        DISPLAY_PREVIEW,
        DISPLAY_DRAG,
        DISPLAY_REMOVE
    };
    eDisplayState displayState = NO_DISPLAY;
    void SetDisplayState(eDisplayState state);

    //controller state. Used only to wrap valuePtr pointer
    //NO_DRAG: do nothing
    //DRAG: store copy of current guides and pointer to a modified guide
    enum eDragState
    {
        NO_DRAG,
        DRAG
    };
    eDragState dragState = NO_DRAG;
    void EnableDrag(DAVA::float32 position);
    void DisableDrag();

    void OnRootControlsChanged(const DAVA::Any& rootControls);

    void SyncGuidesWithValues();

    //returns closest value to given position
    //store current values in cachedValues variable
    //returns cachedValues.end() if closest value is too far or if no values available
    PackageNode::AxisGuides::iterator GetNearestValuePtr(DAVA::float32 position);

    bool IsEnabled() const;
    PackageNode::AxisGuides GetValues() const;
    void SetValues(const PackageNode::AxisGuides& values);

    void CreatePreviewGuide();
    void SetupPreviewGuide(DAVA::float32 position);
    void DragGuide(DAVA::float32 position);

    void RemoveGuide(DAVA::float32 value);
    void RemoveAllGuides();

    bool IsGuidesEnabled() const;
    void SetGuidesEnabled(bool enabled);

    void OnGuidesColorChanged(const DAVA::Any& color);
    void OnPreviewGuideColorChanged(const DAVA::Any& color);

    Guide CreateGuide(const DAVA::Color& color) const;
    void SetGuideColor(QWidget* guide, const DAVA::Color& color) const;

    bool RemoveLastGuideWidget();

    void ResizeGuide(Guide& guide) const;
    void MoveGuide(DAVA::float32 value, Guide& guide) const;

    CentralWidgetData* GetCentralWidgetData() const;
    DocumentData* GetDocumentData() const;

    DAVA::float32 PositionToValue(DAVA::float32 position) const;
    DAVA::float32 ValueToPosition(DAVA::float32 value) const;

    DAVA::Vector2::eAxis orientation = DAVA::Vector2::AXIS_X;

    DAVA::ContextAccessor* accessor = nullptr;
    std::unique_ptr<DAVA::FieldBinder> fieldBinder;
    DAVA::DataWrapper documentDataWrapper;
    DAVA::DataWrapper preferencesDataWrapper;

    //use it only for drag-n-drop
    PackageNode::AxisGuides cachedValues;

    //parent widget of guides
    QWidget* container = nullptr;

    //pointer to currentGuide to modify it value on drag
    PackageNode::AxisGuides::iterator valuePtr;

    //semi-transparent preview guide
    Guide previewGuide;
    QList<Guide> guides;

    //we can not use Show inside Update signal
    DAVA::QtDelayedExecutor delayedExecutor;

    CanvasDataAdapter canvasDataAdapter;
    DAVA::DataWrapper canvasDataAdapterWrapper;
};
