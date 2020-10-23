#pragma once

#include <QString>
#include <QMap>
#include <QList>
#include <QSet>

struct AppVersion;
class QJsonObject;
class QJsonValue;
class BaseTask;

bool FillAppFields(AppVersion* appVer, const QJsonObject& entry, bool toolset);
bool IsToolset(const QString& appName);

class ConfigParser;

struct AppVersion
{
    QString id;
    //can be empty
    QString runPath;
    QString cmd;
    QString url;
    QString buildNum;
    bool isToolSet = false;
};

struct Application
{
    Application()
    {
    }
    Application(const QString& id_)
        : id(id_)
    {
    }

    //in a fact it is an ID to be displayed in UI
    // old name was not changed to save capability with other code
    QString id;

    int GetVerionsCount() const
    {
        return versions.size();
    }
    AppVersion* GetVersion(int index)
    {
        return &versions[index];
    }

    const AppVersion* GetVersion(int index) const
    {
        return &versions[index];
    }

    AppVersion* GetVersion(const QString& versionID);
    AppVersion* GetVersionByNum(const QString& num);
    void RemoveVersion(const QString& versionID);

    QList<AppVersion> versions;
};

struct Branch
{
    Branch()
    {
    }
    Branch(const QString& _id)
        : id(_id)
    {
    }

    QString id;

    int GetAppCount() const
    {
        return applications.size();
    }
    Application* GetApplication(int index)
    {
        return &applications[index];
    }

    const Application* GetApplication(int index) const
    {
        return &applications[index];
    }
    Application* GetApplication(const QString& appID);

    void RemoveApplication(const QString& appID);

    QList<Application> applications;
};

class ConfigParser
{
public:
    ConfigParser();
    void Clear();
    QByteArray Serialize() const;
    void SaveToFile(const QString& filePath) const;

    void InsertApplication(const QString& branchID, const QString& appID, const AppVersion& version);

    void RemoveApplication(const QString& branchID, const QString& appID, const QString& version);

    static QStringList GetToolsetApplications();
    QStringList GetTranslatedToolsetApplications() const;
    int GetBranchCount();
    QString GetBranchID(int index);

    Branch* GetBranch(int branchIndex);
    const Branch* GetBranch(int branchIndex) const;

    Branch* GetBranch(const QString& branch);
    const Branch* GetBranch(const QString& branch) const;

    const Application* GetApplication(const QString& branch, const QString& appID) const;
    Application* GetApplication(const QString& branch, const QString& appID);

    AppVersion* GetAppVersion(const QString& branch, const QString& appID, const QString& ver);
    AppVersion* GetAppVersion(const QString& branch, const QString& appID);

    void RemoveBranch(const QString& branchID);

    QString GetString(const QString& stringID) const;
    const QMap<QString, QString>& GetStrings() const;

    void SetLauncherURL(const QString& url);
    void SetWebpageURL(const QString& url);
    void SetRemoteConfigURL(const QString& url);

    const QString& GetLauncherVersion() const;
    const QString& GetLauncherURL() const;
    const QString& GetWebpageURL() const;

    const QStringList& GetFavorites();

    void MergeBranchesIDs(QSet<QString>& branches);

    void CopyStringsAndFavsFromConfig(ConfigParser* parser);

    void UpdateApplicationsNames();
    bool ParseJSON(const QByteArray& configData, BaseTask* task);

    const QList<Branch>& GetBranches() const;
    QList<Branch>& GetBranches();

    void InsertApplicationImpl(const QString& branchID, const QString& appID, const AppVersion& version);

    bool ExtractLauncherVersionAndURL(const QJsonValue& value);
    bool ExtractLauncherStrings(const QJsonValue& value);
    bool ExtractFavorites(const QJsonValue& value);
    bool ExtractBranches(const QJsonValue& value);

    QString launcherVersion;
    QString launcherURL;
    QString webPageURL;
    QString remoteConfigURL;

    QStringList favorites;

    QList<Branch> branches;
    QMap<QString, QString> strings;
};
