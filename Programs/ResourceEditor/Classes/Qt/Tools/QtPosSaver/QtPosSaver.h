#ifndef __QT_POS_SAVER_H__
#define __QT_POS_SAVER_H__

#include <QObject>
#include <QPointer>

#include "DAVAEngine.h"

class QWidget;
class QMainWindow;
class QSplitter;

class QtPosSaver
: public QObject
{
    Q_OBJECT

public:
    explicit QtPosSaver(QObject* parent = nullptr);
    ~QtPosSaver();

    void Attach(QWidget* widget, const QString& name = QString());
    void SaveValue(const QString& key, const DAVA::VariantType& value);
    DAVA::VariantType LoadValue(const QString& key);

    bool eventFilter(QObject* obj, QEvent* e) override;

private:
    void OnShow();
    void OnHide();

    void SaveGeometry(QWidget* widget);
    void LoadGeometry(QWidget* widget);

    void SaveState(QSplitter* splitter);
    void LoadState(QSplitter* splitter);

    void SaveState(QMainWindow* mainwindow);
    void LoadState(QMainWindow* mainwindow);

    void Save(const QString& key, const QByteArray& data);
    QByteArray Load(const QString& key);

    QPointer<QWidget> attachedWidget;
    QString attachedWidgetName;

private:
    static bool settingsArchiveIsLoaded;
    static DAVA::RefPtr<DAVA::KeyedArchive> settingsArchive;
};

#endif // __QT_POS_SAVER_H__
