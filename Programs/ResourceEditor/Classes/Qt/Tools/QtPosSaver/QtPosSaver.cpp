#include "Classes/Qt/Tools/QtPosSaver/QtPosSaver.h"

#include <QtTools/WidgetHelpers/WidgetStateHelper.h>

#include <QDebug>
#include <QWidget>
#include <QSplitter>
#include <QMainWindow>
#include <QEvent>

bool QtPosSaver::settingsArchiveIsLoaded = false;
DAVA::RefPtr<DAVA::KeyedArchive> QtPosSaver::settingsArchive(nullptr);

QtPosSaver::QtPosSaver(QObject* parent)
    : QObject(parent)
{
    if (!settingsArchiveIsLoaded)
    {
        settingsArchive.Set(new DAVA::KeyedArchive());
        settingsArchive->Load("~doc:/ResourceEditorPos.archive");
        settingsArchiveIsLoaded = true;
    }
    else
    {
        settingsArchive->Retain();
    }

    auto w = qobject_cast<QWidget*>(parent);
    if (w != nullptr)
    {
        Attach(w);
    }
}

QtPosSaver::~QtPosSaver()
{
    if (settingsArchiveIsLoaded)
    {
        if (!attachedWidget.isNull() && !attachedWidgetName.isEmpty() && attachedWidget->isVisible())
        {
            OnHide();
        }

        if (!attachedWidget.isNull())
        {
            attachedWidget->removeEventFilter(this);
        }

        if (1 == settingsArchive->GetRetainCount())
        {
            settingsArchive->Save("~doc:/ResourceEditorPos.archive");
            settingsArchive.Set(nullptr);
            settingsArchiveIsLoaded = false;
        }
        else
        {
            settingsArchive->Release();
        }
    }
}

void QtPosSaver::Attach(QWidget* widget, const QString& name)
{
    attachedWidget = widget;

    if (nullptr != attachedWidget)
    {
        if (name.isEmpty())
        {
            attachedWidgetName = attachedWidget->objectName();
        }
        else
        {
            attachedWidgetName = name;
        }

        attachedWidget->installEventFilter(this);
        OnShow();
    }
}

void QtPosSaver::OnShow()
{
    auto mainWindow = qobject_cast<QMainWindow*>(attachedWidget);
    if (mainWindow != nullptr)
    {
        LoadState(mainWindow);
    }

    auto splitter = qobject_cast<QSplitter*>(attachedWidget);
    if (splitter != nullptr)
    {
        LoadState(splitter);
    }

    auto widget = qobject_cast<QWidget*>(attachedWidget);
    if (widget != nullptr && mainWindow == nullptr && splitter == nullptr)
    {
        LoadGeometry(widget);
    }

    if (!attachedWidget.isNull())
    {
        attachedWidget->updateGeometry();
    }
}

void QtPosSaver::OnHide()
{
    auto mainWindow = qobject_cast<QMainWindow*>(attachedWidget);
    if (mainWindow != nullptr)
    {
        SaveState(mainWindow);
    }

    auto splitter = qobject_cast<QSplitter*>(attachedWidget);
    if (splitter != nullptr)
    {
        SaveState(splitter);
    }

    auto widget = qobject_cast<QWidget*>(attachedWidget);
    if (widget != nullptr && mainWindow == nullptr && splitter == nullptr)
    {
        SaveGeometry(widget);
    }
}

void QtPosSaver::SaveGeometry(QWidget* widget)
{
    if (nullptr != widget && !attachedWidgetName.isEmpty())
    {
        const auto normalKey = QString("%1-geometry-%2").arg(attachedWidgetName).arg(widget->objectName());
        Save(normalKey, widget->saveGeometry());

        const auto isMaximizedKey = QString("%1-maximized-%2").arg(attachedWidgetName).arg(widget->objectName());
        QByteArray mState(1, static_cast<char>(widget->isMaximized()));
        Save(isMaximizedKey, mState);
    }
}

void QtPosSaver::LoadGeometry(QWidget* widget)
{
    if (nullptr != widget && !attachedWidgetName.isEmpty())
    {
        auto helper = WidgetStateHelper::create(widget);
        helper->setTrackedEvents(WidgetStateHelper::ScaleOnDisplayChange);

        QString normalKey = QString("%1-geometry-%2").arg(attachedWidgetName).arg(widget->objectName());
        QString isMaximizedKey = QString("%1-maximized-%2").arg(attachedWidgetName).arg(widget->objectName());
        QByteArray mState = Load(isMaximizedKey);

        QByteArray geometry = Load(normalKey);
        widget->restoreGeometry(geometry);

        if (!mState.isEmpty() && mState.at(0) != 0)
        {
            const auto f = helper->getTrackedEvents() | WidgetStateHelper::MaximizeOnShowOnce;
            helper->setTrackedEvents(f);
        }
    }
}

void QtPosSaver::SaveState(QSplitter* splitter)
{
    if (nullptr != splitter && !attachedWidgetName.isEmpty())
    {
        const auto splitterKey = QString("%1-splitter-%2").arg(attachedWidgetName).arg(splitter->objectName());
        Save(splitterKey, splitter->saveState());
    }
}

void QtPosSaver::LoadState(QSplitter* splitter)
{
    if (nullptr != splitter && !attachedWidgetName.isEmpty())
    {
        const auto splitterKey = QString("%1-splitter-%2").arg(attachedWidgetName).arg(splitter->objectName());
        splitter->restoreState(Load(splitterKey));
    }
}

void QtPosSaver::SaveState(QMainWindow* mainwindow)
{
    SaveGeometry(mainwindow);

    if (nullptr != mainwindow && !attachedWidgetName.isEmpty())
    {
        const auto mainWindowKey = QString("%1-mainwindow-%2").arg(attachedWidgetName).arg(mainwindow->objectName());
        Save(mainWindowKey, mainwindow->saveState());
    }
}

void QtPosSaver::LoadState(QMainWindow* mainwindow)
{
    LoadGeometry(mainwindow);

    if (nullptr != mainwindow && !attachedWidgetName.isEmpty())
    {
        const auto mainWindowKey = QString("%1-mainwindow-%2").arg(attachedWidgetName).arg(mainwindow->objectName());
        mainwindow->restoreState(Load(mainWindowKey));
    }
}

void QtPosSaver::SaveValue(const QString& key, const DAVA::VariantType& value)
{
    if (settingsArchiveIsLoaded && !key.isEmpty())
    {
        const auto valueKey = QString("%1-%2").arg(attachedWidgetName).arg(key);
        settingsArchive->SetVariant(valueKey.toStdString(), value);
    }
}

DAVA::VariantType QtPosSaver::LoadValue(const QString& key)
{
    DAVA::VariantType v;

    if (settingsArchiveIsLoaded && !key.isEmpty())
    {
        const auto valueKey = QString("%1-%2").arg(attachedWidgetName).arg(key);
        const auto val = settingsArchive->GetVariant(valueKey.toStdString());
        if (nullptr != val)
        {
            v = *val;
        }
    }

    return v;
}

bool QtPosSaver::eventFilter(QObject* obj, QEvent* e)
{
    if (obj == attachedWidget)
    {
        switch (e->type())
        {
        case QEvent::Show:
            OnShow();
            break;
        case QEvent::Hide:
            OnHide();
            break;

        default:
            break;
        }
    }

    return QObject::eventFilter(obj, e);
}

void QtPosSaver::Save(const QString& key, const QByteArray& data)
{
    if (settingsArchiveIsLoaded && !key.isEmpty() && !data.isEmpty())
    {
        settingsArchive->SetByteArray(key.toStdString(), reinterpret_cast<const DAVA::uint8*>(data.constData()), data.size());
    }
}

QByteArray QtPosSaver::Load(const QString& key)
{
    QByteArray data;

    if (settingsArchiveIsLoaded && !key.isEmpty())
    {
        DAVA::String keyStr = key.toStdString();
        DAVA::int32 sz = settingsArchive->GetByteArraySize(keyStr);
        const DAVA::uint8* dt = settingsArchive->GetByteArray(keyStr);

        if (nullptr != dt)
        {
            data.append(reinterpret_cast<const char*>(dt), sz);
        }
    }

    return data;
}
