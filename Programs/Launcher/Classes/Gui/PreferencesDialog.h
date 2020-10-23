#pragma once

#include "ui_preferencesdialog.h"
#include "Core/UrlsHolder.h"

class FileManager;
class UrlsHolder;
class BAManagerClient;
class ConfigRefresher;
struct ApplicationContext;

class PreferencesDialog : public QDialog, private Ui::PreferencesDialog
{
    Q_OBJECT

public:
    static void ShowPreferencesDialog(ApplicationContext* context, ConfigRefresher* configRefresher, QWidget* parent = nullptr);

    static void SavePreferences(ApplicationContext* context, BAManagerClient* commandListener, ConfigRefresher* configRefresher);
    static void LoadPreferences(ApplicationContext* context);
    static void LoadPreferences(ApplicationContext* context, BAManagerClient* commandListener, ConfigRefresher* configRefresher);

private slots:
    void OnButtonCopyURLClicked();
    void OnButtonChooseFilesPathClicked();
    void ProcessSaveButtonEnabled();
    void OnServerHostNameChanged(const QString& name);

private:
    PreferencesDialog(QWidget* parent = nullptr);
    void Init(ApplicationContext* context, ConfigRefresher* configRefresher);
    void AcceptData();

    ApplicationContext* context = nullptr;
    ConfigRefresher* configRefresher = nullptr;

    QMap<UrlsHolder::eURLType, QLabel*> urlWidgets;
    QMap<UrlsHolder::eURLType, QPushButton*> copyURLWidgets;
};
