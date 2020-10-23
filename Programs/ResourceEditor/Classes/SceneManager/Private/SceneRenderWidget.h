#pragma once

#include <TArc/DataProcessing/DataWrapper.h>
#include <TArc/DataProcessing/DataListener.h>
#include <TArc/Utils/QtConnections.h>

#include <Engine/Qt/IClientDelegate.h>
#include <Engine/Qt/RenderWidget.h>

#include <Reflection/Reflection.h>
#include <UI/UIScreen.h>
#include <UI/UI3DView.h>

#include <Base/BaseTypes.h>
#include <Base/Any.h>
#include <Base/RefPtr.h>

#include <QFrame>

namespace DAVA
{
class RenderWidget;
class ContextAccessor;
class SceneEditor2;
class SelectableGroup;
}

class SceneRenderWidget : public QFrame, private DAVA::DataListener,
                          private DAVA::IClientDelegate,
                          public DAVA::TrackedObject
{
    Q_OBJECT
public:
    class IWidgetDelegate
    {
    public:
        virtual bool OnCloseSceneRequest(DAVA::uint64 id) = 0;

        virtual void OnDragEnter(QObject* target, QDragEnterEvent* event) = 0;
        virtual void OnDragMove(QObject* target, QDragMoveEvent* event) = 0;
        virtual void OnDrop(QObject* target, QDropEvent* event) = 0;
    };

    SceneRenderWidget(DAVA::ContextAccessor* accessor, DAVA::RenderWidget* renderWidget, IWidgetDelegate* widgetDelegate);
    ~SceneRenderWidget();

private:
    void InitDavaUI();
    void UninitDavaUI();

    void OnDataChanged(const DAVA::DataWrapper& wrapper, const DAVA::Vector<DAVA::Any>& fields) override;
    void OnRenderWidgetResized(DAVA::uint32 w, DAVA::uint32 h);
    void OnCloseTab(DAVA::uint64 id);
    void OnDeleteSelection();
    void OnMouseOverSelection(DAVA::SceneEditor2* scene, const DAVA::SelectableGroup* objects);

    bool eventFilter(QObject* object, QEvent* event) override;
    void OnDragEntered(QDragEnterEvent* e) override;
    void OnDragMoved(QDragMoveEvent* e) override;
    void OnDrop(QDropEvent* e) override;

private:
    DAVA::ContextAccessor* accessor = nullptr;
    DAVA::DataWrapper activeSceneWrapper;

    DAVA::RefPtr<DAVA::UIScreen> davaUIScreen;
    DAVA::RefPtr<DAVA::UI3DView> dava3DView;
    const int davaUIScreenID = 0;
    const int dava3DViewMargin = 3;

    DAVA::QtConnections connections;
    DAVA::RenderWidget* renderWidget = nullptr;

    IWidgetDelegate* widgetDelegate = nullptr;
};
