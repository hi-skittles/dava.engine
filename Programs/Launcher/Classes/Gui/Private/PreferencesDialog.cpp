#include "Gui/PreferencesDialog.h"

#include "Core/BAManagerClient.h"
#include "Core/ConfigRefresher.h"
#include "Core/ApplicationContext.h"

#include "Utils/FileManager.h"
#include "Utils/ErrorMessenger.h"

#include <QFileDialog>
#include <QJsonObject>
#include <QJsonDocument>
#include <QApplication>
#include <QClipboard>
#include <QStandardPaths>

namespace PreferencesDialogDetails
{
const char* propertyKey = "urlType";
const char* settingsFileName = "LauncherPreferences.json";
const char* filesDirectoryKey = "storage path";
const char* launcherProtocolKey = "BA-manager key";
const char* autorefreshEnabledKey = "autorefresh enabled";
const char* autorefreshTimeoutKey = "autorefresh timeout";
const char* serverHostNameKey = "Ba-manager url";
const char* useTestAPIKey = "use test API";
const char* buildAgentKey = "is build agent";
};

void PreferencesDialog::ShowPreferencesDialog(ApplicationContext* context, ConfigRefresher* refresher, QWidget* parent)
{
    PreferencesDialog dialog(parent);
    dialog.Init(context, refresher);
    if (dialog.exec() == QDialog::Accepted)
    {
        dialog.AcceptData();
    }
}

void PreferencesDialog::SavePreferences(ApplicationContext* context, BAManagerClient* commandListener, ConfigRefresher* refresher)
{
    QJsonObject rootObject;
    rootObject[PreferencesDialogDetails::filesDirectoryKey] = context->fileManager.GetFilesDirectory();
    rootObject[PreferencesDialogDetails::launcherProtocolKey] = commandListener->GetProtocolKey();
    rootObject[PreferencesDialogDetails::serverHostNameKey] = context->urlsHolder.GetServerHostName();
    rootObject[PreferencesDialogDetails::useTestAPIKey] = context->urlsHolder.IsTestAPIUsed();

    rootObject[PreferencesDialogDetails::autorefreshEnabledKey] = refresher->IsEnabled();
    rootObject[PreferencesDialogDetails::autorefreshTimeoutKey] = refresher->GetTimeout();
    rootObject[PreferencesDialogDetails::buildAgentKey] = context->isBuildAgent;

    QJsonDocument document(rootObject);
    QString filePath = context->fileManager.GetDocumentsDirectory() + PreferencesDialogDetails::settingsFileName;
    QFile settingsFile(filePath);
    if (settingsFile.open(QFile::WriteOnly | QFile::Truncate))
    {
        settingsFile.write(document.toJson());
    }
    else
    {
        ErrorMessenger::ShowErrorMessage(ErrorMessenger::ERROR_FILE, QObject::tr("Can not write to file %1").arg(filePath));
    }
}

void PreferencesDialog::LoadPreferences(ApplicationContext* context)
{
    QString filePath = context->fileManager.GetDocumentsDirectory() + PreferencesDialogDetails::settingsFileName;
    QFile settingsFile(filePath);
    QJsonDocument document;
    if (settingsFile.exists())
    {
        if (settingsFile.open(QFile::ReadOnly))
        {
            QByteArray data = settingsFile.readAll();
            document = QJsonDocument::fromJson(data);
        }
        else
        {
            ErrorMessenger::ShowErrorMessage(ErrorMessenger::ERROR_FILE, QObject::tr("Can not open file %1").arg(filePath));
        }
    }

    QJsonObject rootObject = document.object();
    QJsonValue serverHostNameValue = rootObject[PreferencesDialogDetails::serverHostNameKey];
    if (serverHostNameValue.isString())
    {
        context->urlsHolder.SetServerHostName(serverHostNameValue.toString());
    }

    QJsonValue canUseTestAPIValue = rootObject[PreferencesDialogDetails::useTestAPIKey];
    if (canUseTestAPIValue.isBool())
    {
        context->urlsHolder.SetUseTestAPI(canUseTestAPIValue.toBool());
    }

    QJsonValue isBuildAgentValue = rootObject[PreferencesDialogDetails::buildAgentKey];
    if (isBuildAgentValue.isBool())
    {
        context->isBuildAgent = isBuildAgentValue.toBool();
    }

    QJsonValue filesDirValue = rootObject[PreferencesDialogDetails::filesDirectoryKey];
    if (filesDirValue.isString())
    {
        QString filesDir = filesDirValue.toString();
        context->fileManager.SetFilesDirectory(filesDir, false);
        
#ifdef Q_OS_MAC
        if (filesDir.startsWith(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)))
        {
            ErrorMessenger::ShowErrorMessage(ErrorMessenger::ERROR_FILE, QObject::tr("Storage path is located inside of '%1'\nIt's recommended to relocate storage outside of this path").arg(filesDir));
        }
#endif
    }
}

void PreferencesDialog::LoadPreferences(ApplicationContext* context, BAManagerClient* commandListener, ConfigRefresher* refresher)
{
    LoadPreferences(context);

    QString filePath = context->fileManager.GetDocumentsDirectory() + PreferencesDialogDetails::settingsFileName;
    QFile settingsFile(filePath);
    QJsonDocument document;
    if (settingsFile.exists())
    {
        if (settingsFile.open(QFile::ReadOnly))
        {
            QByteArray data = settingsFile.readAll();
            document = QJsonDocument::fromJson(data);
        }
        else
        {
            ErrorMessenger::ShowErrorMessage(ErrorMessenger::ERROR_FILE, QObject::tr("Can not open file %1").arg(filePath));
        }
    }

    QJsonObject rootObject = document.object();

    QJsonValue protocolKeyValue = rootObject[PreferencesDialogDetails::launcherProtocolKey];
    if (protocolKeyValue.isString())
    {
        commandListener->SetProtocolKey(protocolKeyValue.toString());
    }

    QJsonValue autorefreshTimeoutValue = rootObject[PreferencesDialogDetails::autorefreshTimeoutKey];
    if (autorefreshTimeoutValue.isDouble())
    {
        refresher->SetTimeout(autorefreshTimeoutValue.toInt());
    }

    QJsonValue autorefreshEnabledValue = rootObject[PreferencesDialogDetails::autorefreshEnabledKey];
    if (autorefreshEnabledValue.isBool())
    {
        refresher->SetEnabled(autorefreshEnabledValue.toBool());
    }
}

PreferencesDialog::PreferencesDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUi(this);
    connect(pushButton_selectStorageDir, &QPushButton::clicked, this, &PreferencesDialog::OnButtonChooseFilesPathClicked);
    connect(lineEdit_launcherDataPath, &QLineEdit::textChanged, this, &PreferencesDialog::ProcessSaveButtonEnabled);
    connect(lineEdit_serverHostName, &QLineEdit::textChanged, this, &PreferencesDialog::ProcessSaveButtonEnabled);
}

void PreferencesDialog::Init(ApplicationContext* context_, ConfigRefresher* refresher_)
{
    context = context_;
    configRefresher = refresher_;
    Q_ASSERT(context != nullptr);
    Q_ASSERT(configRefresher != nullptr);

    lineEdit_launcherDataPath->setText(context->fileManager.GetFilesDirectory());

    lineEdit_serverHostName->setText(context->urlsHolder.GetServerHostName());
    connect(lineEdit_serverHostName, &QLineEdit::textChanged, this, &PreferencesDialog::OnServerHostNameChanged);

    label_currentPlatformBuildsLabel->setText(platformString + QString(" builds URL"));

    urlWidgets[UrlsHolder::LauncherInfoURL] = label_launcherInfoURL;
    urlWidgets[UrlsHolder::LauncherTestInfoURL] = label_launcherInfoTestURL;
    urlWidgets[UrlsHolder::StringsURL] = label_launcherMetaInfoURL;
    urlWidgets[UrlsHolder::FavoritesURL] = label_launcherFavoritesURL;
    urlWidgets[UrlsHolder::AllBuildsCurrentPlatformURL] = label_currentPlatformBuildsURL;
    urlWidgets[UrlsHolder::AllBuildsIOSURL] = label_IOSBuildsURL;
    urlWidgets[UrlsHolder::AllBuildsAndroidURL] = label_androidBuildsURL;
    urlWidgets[UrlsHolder::AllBuildsUWPURL] = label_UWPBuildsURL;

    copyURLWidgets[UrlsHolder::LauncherInfoURL] = pushButton_copyLauncherInfoURL;
    copyURLWidgets[UrlsHolder::LauncherTestInfoURL] = pushButton_copyLauncherTestInfoURL;
    copyURLWidgets[UrlsHolder::StringsURL] = pushButton_copyMetaInfoURL;
    copyURLWidgets[UrlsHolder::FavoritesURL] = pushButton_copyFavoritesURL;
    copyURLWidgets[UrlsHolder::AllBuildsCurrentPlatformURL] = pushButton_copyCurrentPlatformBuildsURL;
    copyURLWidgets[UrlsHolder::AllBuildsAndroidURL] = pushButton_copyAndroidBuildsURL;
    copyURLWidgets[UrlsHolder::AllBuildsIOSURL] = pushButton_copyIOSBuildsURL;
    copyURLWidgets[UrlsHolder::AllBuildsUWPURL] = pushButton_copyUWPBuildsURL;

    for (int i = UrlsHolder::LauncherInfoURL; i < UrlsHolder::URLTypesCount; ++i)
    {
        UrlsHolder::eURLType type = static_cast<UrlsHolder::eURLType>(i);
        urlWidgets[type]->setProperty(PreferencesDialogDetails::propertyKey, i);
        copyURLWidgets[type]->setProperty(PreferencesDialogDetails::propertyKey, i);

        connect(copyURLWidgets[type], &QPushButton::clicked, this, &PreferencesDialog::OnButtonCopyURLClicked);
    }
    checkBox_useTestAPI->setChecked(context->urlsHolder.IsTestAPIUsed());

    checkBox_autorefreshEnabled->setChecked(configRefresher->IsEnabled());
    int minTimeout = configRefresher->GetMinimumTimeout();
    int maxTimeout = configRefresher->GetMaximumTimeout();
    spinBox_autorefreshTimeout->setRange(minTimeout, maxTimeout);
    spinBox_autorefreshTimeout->setEnabled(true);
    spinBox_autorefreshTimeout->setValue(configRefresher->GetTimeout());
    spinBox_autorefreshTimeout->setMinimum(configRefresher->GetMinimumTimeout());
    spinBox_autorefreshTimeout->setMaximum(configRefresher->GetMaximumTimeout());

    checkBox_buildAgent->setChecked(context->isBuildAgent);
    OnServerHostNameChanged(lineEdit_serverHostName->text());
}

void PreferencesDialog::AcceptData()
{
    Q_ASSERT(context != nullptr);

    context->fileManager.SetFilesDirectory(lineEdit_launcherDataPath->text(), true);

    context->urlsHolder.SetServerHostName(lineEdit_serverHostName->text());
    context->urlsHolder.SetUseTestAPI(checkBox_useTestAPI->isChecked());
    context->isBuildAgent = checkBox_buildAgent->isChecked();

    configRefresher->SetEnabled(checkBox_autorefreshEnabled->isChecked());
    configRefresher->SetTimeout(spinBox_autorefreshTimeout->value());
}

void PreferencesDialog::OnButtonChooseFilesPathClicked()
{
    QString newPath = QFileDialog::getExistingDirectory(parentWidget(), "Choose new directory", lineEdit_launcherDataPath->text());
    if (!newPath.isEmpty())
    {
        lineEdit_launcherDataPath->setText(newPath);
    }
}

void PreferencesDialog::ProcessSaveButtonEnabled()
{
    bool enabled = (lineEdit_serverHostName->text().isEmpty() == false);
    if (enabled)
    {
        QString path = lineEdit_launcherDataPath->text();
        QFileInfo fileInfo(path);
        enabled = fileInfo.isDir();
    }
    QPushButton* button = buttonBox->button(QDialogButtonBox::Save);
    Q_ASSERT(button != nullptr);
    button->setEnabled(enabled);
}

void PreferencesDialog::OnServerHostNameChanged(const QString& name)
{
    for (int i = UrlsHolder::LauncherInfoURL; i < UrlsHolder::URLTypesCount; ++i)
    {
        UrlsHolder::eURLType type = static_cast<UrlsHolder::eURLType>(i);
        QString text = name + context->urlsHolder.GetURL(type);

        urlWidgets[type]->setText("<a href=\"" + text + "\">" + text + "</a>");
    }
}

void PreferencesDialog::OnButtonCopyURLClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    Q_ASSERT(button != nullptr);
    bool ok;
    int typeInt = button->property(PreferencesDialogDetails::propertyKey).toInt(&ok);
    UrlsHolder::eURLType type = static_cast<UrlsHolder::eURLType>(typeInt);
    Q_ASSERT(ok);
    Q_ASSERT(urlWidgets.contains(type));
    QString text = lineEdit_serverHostName->text() + context->urlsHolder.GetURL(type);
    QApplication::clipboard()->setText(text);
}
