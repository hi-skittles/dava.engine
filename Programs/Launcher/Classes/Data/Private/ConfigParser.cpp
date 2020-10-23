#include "Data/ConfigParser.h"
#include "Core/CommonTasks/BaseTask.h"

#include "Utils/ErrorMessenger.h"

#include "defines.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>
#include <QDebug>
#include <QJsonParseError>
#include <QRegularExpression>

#include <functional>

namespace ConfigParserDetails
{
QString ProcessID(const QString& id)
{
    QRegularExpressionMatch match;
    QRegularExpression regex("\\[\\d+\\_\\d+\\_\\d+\\]");
    int index = id.indexOf(regex, 0, &match);
    if (index == -1)
    {
        int digitIndex = id.indexOf(QRegularExpression("\\d+"));
        if (digitIndex == -1)
        {
            return id;
        }
        return id.right(id.length() - digitIndex);
    }
    QString version = match.captured();
    int versionLength = version.length();
    QStringList digits = version.split(QRegularExpression("\\D+"), QString::SkipEmptyParts);
    version = digits.join(".");
    QString dateTime = id.right(id.length() - versionLength - index);
    QString extension(".txt");
    if (dateTime.endsWith(extension))
    {
        dateTime.chop(extension.length());
    }
    QRegularExpression timeRegex("\\_\\d+\\_\\d+\\_\\d+");
    if (dateTime.indexOf(timeRegex, 0, &match) != -1)
    {
        QString time = match.captured();
        time = time.split(QRegularExpression("\\D+"), QString::SkipEmptyParts).join(".");
        dateTime.replace(timeRegex, "_" + time);
    }
    QString result = version + dateTime;
    return result;
}

bool ExtractApp(const QString& appName, const QJsonObject& entry, Branch* branch, bool toolset)
{
    if (appName.isEmpty())
    {
        return false;
    }
    Application* app = branch->GetApplication(appName);
    if (app == nullptr)
    {
        branch->applications.append(Application(appName));
        app = &branch->applications.last();
    }
    QString buildNum = entry["build_num"].toString();
    AppVersion* appVer = nullptr;
    if (buildNum.isEmpty() == false)
    {
        appVer = app->GetVersionByNum(buildNum);
    }
    if (appVer == nullptr)
    {
        QString buildType = entry["build_type"].toString();
        if (buildType.isEmpty())
        {
            return false;
        }
        appVer = app->GetVersion(buildType);
    }
    if (appVer == nullptr)
    {
        app->versions.append(AppVersion());
        appVer = &app->versions.last();
    }
    return FillAppFields(appVer, entry, toolset);
}
}

bool LessThan(const AppVersion& leftVer, const AppVersion& rightVer);

bool ConfigParser::ExtractLauncherVersionAndURL(const QJsonValue& value)
{
    QJsonObject launcherObject = value.toObject();
    QJsonObject platformObject = launcherObject[platformString].toObject();

    QJsonValue versionValue = platformObject["version"];
    if (versionValue.isString())
    {
        launcherVersion = versionValue.toString();
    }
    QJsonValue urlValue = platformObject["url"];
    if (urlValue.isString())
    {
        launcherURL = urlValue.toString();
    }
    QJsonValue newsValue = launcherObject["news"];
    if (newsValue.isString())
    {
        webPageURL = newsValue.toString();
    }
    return !launcherObject.isEmpty();
}

bool ConfigParser::ExtractLauncherStrings(const QJsonValue& value)
{
    QJsonArray array = value.toArray();
    bool isValid = !array.isEmpty();
    for (const QJsonValueRef& ref : array)
    {
        QJsonObject entry = ref.toObject();
        if (entry["os"].toString() != platformString)
        {
            continue;
        }
        QString key = entry["build_tag"].toString();
        QString stringValue = entry["build_name"].toString();
        isValid &= !key.isEmpty() && !stringValue.isEmpty();
        strings[key] = stringValue;
    }
    return isValid;
}

bool ConfigParser::ExtractFavorites(const QJsonValue& value)
{
    QJsonArray array = value.toArray();
    bool isValid = !array.isEmpty();
    for (const QJsonValueRef& ref : array)
    {
        QJsonObject entry = ref.toObject();
        if (entry["favourite"].toString() != "1")
        {
            continue;
        }
        QString fave = entry["branch_name"].toString();
        isValid &= !fave.isEmpty();
        //favorites list can not be large
        if (!favorites.contains(fave))
        {
            favorites.append(fave);
        }
    }

    return isValid;
}

bool ConfigParser::ExtractBranches(const QJsonValue& value)
{
    QJsonArray array = value.toArray();
    //this array can be empty
    bool isValid = true;
    for (const QJsonValueRef& ref : array)
    {
        QJsonObject entry = ref.toObject();
        QString branchNameID = "branchName";
        //now ASK builds without branch name
        if (!entry[branchNameID].isString())
        {
            isValid = false;
            continue;
        }
        QString branchName = entry[branchNameID].toString();

        Branch* branch = nullptr;
        //foreach will cause deep copy in this case
        int branchCount = branches.size();
        for (int i = 0; i < branchCount; ++i)
        {
            if (branches[i].id == branchName)
            {
                branch = &branches[i];
            }
        }
        if (branch == nullptr)
        {
            branches.append(Branch(branchName));
            branch = &branches.last();
        }

        QString appName = entry["build_name"].toString();
        if (IsToolset(appName))
        {
            for (const QString& toolsetApp : ConfigParser::GetToolsetApplications())
            {
                isValid &= ConfigParserDetails::ExtractApp(toolsetApp, entry, branch, true);
            }
        }
        else
        {
            isValid &= ConfigParserDetails::ExtractApp(appName, entry, branch, false);
        }
    }
    //hot fix to sort downloaded items without rewriting mainWindow
    for (auto branchIter = branches.begin(); branchIter != branches.end(); ++branchIter)
    {
        QList<Application>& apps = branchIter->applications;
        qSort(apps.begin(), apps.end(), [](const Application& appLeft, const Application& appRight)
              {
                  return appLeft.id < appRight.id;
              });
    }

    return isValid;
}

bool IsToolset(const QString& appName)
{
    return appName.startsWith("toolset", Qt::CaseInsensitive);
}

bool IsBuildSupported(const QString& url)
{
    static QStringList supportedExtensions = {
        ".zip", ".ipa"
    };

    for (const QString& ext : supportedExtensions)
    {
        if (url.endsWith(ext))
        {
            return true;
        }
    }
    return false;
}

bool FillAppFields(AppVersion* appVer, const QJsonObject& entry, bool toolset)
{
    QString url = entry["artifacts"].toString();
    QString buildType = entry["build_type"].toString();

    //remember num to fill it later
    appVer->buildNum = entry["build_num"].toString();
    if (appVer->id.isEmpty() || url.endsWith(".zip") || buildType.startsWith("Desc"))
    {
        appVer->id = ConfigParserDetails::ProcessID(buildType);
    }

    if (IsBuildSupported(url) == false)
    {
        //this is valid situation
        return true;
    }

    appVer->url = url;
    appVer->runPath = toolset ? "" : entry["exe_location"].toString();
    appVer->isToolSet = toolset;
    return !appVer->url.isEmpty();
}

AppVersion* Application::GetVersion(const QString& versionID)
{
    int versCount = versions.size();
    for (int i = 0; i < versCount; ++i)
        if (versions[i].id == versionID)
            return &versions[i];

    return 0;
}

AppVersion* Application::GetVersionByNum(const QString& num)
{
    auto iter = std::find_if(versions.begin(), versions.end(), [num](const AppVersion& ver) {
        return ver.buildNum == num;
    });
    if (iter == versions.end())
    {
        return nullptr;
    }
    return &(*iter);
}

void Application::RemoveVersion(const QString& versionID)
{
    int index = -1;
    int versCount = versions.size();
    for (int i = 0; i < versCount; ++i)
    {
        if (versions[i].id == versionID)
        {
            index = i;
            break;
        }
    }
    if (index != -1)
        versions.removeAt(index);
}

Application* Branch::GetApplication(const QString& appID)
{
    int appCount = applications.size();
    for (int i = 0; i < appCount; ++i)
        if (applications[i].id == appID)
            return &applications[i];

    return 0;
}

void Branch::RemoveApplication(const QString& appID)
{
    int index = -1;
    int appCount = applications.size();
    for (int i = 0; i < appCount; ++i)
    {
        if (applications[i].id == appID)
        {
            index = i;
            break;
        }
    }
    if (index != -1)
        applications.removeAt(index);
}

ConfigParser::ConfigParser()
    : launcherVersion(LAUNCHER_VER)
    , webPageURL("")
    , remoteConfigURL("")
{
}

void ConfigParser::Clear()
{
    launcherVersion = LAUNCHER_VER;
    launcherURL.clear();
    webPageURL.clear();
    remoteConfigURL.clear();

    favorites.clear();

    branches.clear();
    strings.clear();
}

bool ConfigParser::ParseJSON(const QByteArray& configData, BaseTask* task)
{
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(configData, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        //this is not JSON
        return false;
    }
    QJsonObject rootObj = document.object();
    if (rootObj.keys().isEmpty())
    {
        task->SetError(QObject::tr("Config parser: Got an empty config from server "));
    }

    using namespace std::placeholders;
    using ParserFn = std::function<bool(const QJsonValue&)>;
    QMap<QString, ParserFn> parsers = {
        { "launcher", std::bind(&ConfigParser::ExtractLauncherVersionAndURL, this, _1) },
        { "seo_list", std::bind(&ConfigParser::ExtractLauncherStrings, this, _1) },
        { "branches", std::bind(&ConfigParser::ExtractFavorites, this, _1) },
        { "builds", std::bind(&ConfigParser::ExtractBranches, this, _1) }
    };

    bool haveUsefulInformation = false;
    for (const QString& key : rootObj.keys())
    {
        if (parsers.contains(key))
        {
            QJsonValue value = rootObj.value(key);
            ParserFn parser = parsers[key];
            if (parser(value))
            {
                haveUsefulInformation = true;
            }
            else
            {
                task->SetError(QObject::tr("Got wrong config page: %1").arg(key));
            }
        }
    }
    if (haveUsefulInformation)
    {
        for (Branch& branch : branches)
        {
            for (Application& application : branch.applications)
            {
                qSort(application.versions.begin(), application.versions.end(), LessThan);
            }
        }
    }
    return haveUsefulInformation;
}

const QList<Branch>& ConfigParser::GetBranches() const
{
    return branches;
}

QList<Branch>& ConfigParser::GetBranches()
{
    return branches;
}

void ConfigParser::CopyStringsAndFavsFromConfig(ConfigParser* parser)
{
    QMap<QString, QString>::ConstIterator it = parser->strings.begin();
    QMap<QString, QString>::ConstIterator itEnd = parser->strings.end();
    for (; it != itEnd; ++it)
        strings[it.key()] = it.value();
    if (!parser->favorites.isEmpty())
    {
        favorites = parser->favorites;
    }
}

void ConfigParser::UpdateApplicationsNames()
{
    for (auto branchIter = branches.begin(); branchIter != branches.end(); ++branchIter)
    {
        for (auto appIter = branchIter->applications.begin(); appIter != branchIter->applications.end(); ++appIter)
        {
            auto stringsIter = strings.find(appIter->id);
            if (stringsIter != strings.end())
            {
                appIter->id = *stringsIter;
            }
        }
    }
}

QByteArray ConfigParser::Serialize() const
{
    QJsonObject rootObject;

    QJsonObject launcherObject;
    launcherObject["url"] = webPageURL;
    rootObject["launcher"] = launcherObject;

    QJsonArray favoritesArray;
    for (const QString& favoriteBranch : favorites)
    {
        QJsonObject favObject = {
            { "branch_name", favoriteBranch },
            { "favourite", "1" }
        };
        favoritesArray.append(favObject);
    }
    rootObject["branches"] = favoritesArray;

    QJsonArray stringsArray;
    QMap<QString, QString>::ConstIterator it = strings.constBegin();
    QMap<QString, QString>::ConstIterator itEnd = strings.constEnd();
    for (; it != itEnd; ++it)
    {
        QJsonObject stringsObj = {
            { "build_tag", it.key() },
            { "build_name", it.value() },
            { "os", platformString }
        };
        stringsArray.append(stringsObj);
    }
    rootObject["seo_list"] = stringsArray;

    QJsonArray buildsArray;
    for (int i = 0; i < branches.size(); ++i)
    {
        const Branch* branch = GetBranch(i);
        for (int j = 0; j < branch->GetAppCount(); ++j)
        {
            const Application* app = branch->GetApplication(j);
            for (int k = 0; k < app->GetVerionsCount(); ++k)
            {
                const AppVersion* ver = app->GetVersion(k);
                QString appName = ver->isToolSet ? "ToolSet" : app->id;
                QJsonObject buildObj = {
                    { "build_num", ver->buildNum },
                    { "build_type", ver->id },
                    { "build_name", appName },
                    { "branchName", branch->id },
                    { "artifacts", ver->url },
                    { "exe_location", ver->runPath }
                };
                buildsArray.append(buildObj);
            }
        }
    }
    rootObject["builds"] = buildsArray;

    QJsonDocument document(rootObject);
    return document.toJson();
}

void ConfigParser::SaveToFile(const QString& filePath) const
{
    QFile file(filePath);
    if (file.open(QFile::WriteOnly | QFile::Truncate))
    {
        QByteArray data = Serialize();
        file.write(data);
    }
    file.close();
}

void ConfigParser::RemoveBranch(const QString& branchID)
{
    int index = -1;
    int branchesCount = branches.size();
    for (int i = 0; i < branchesCount; ++i)
    {
        if (branches[i].id == branchID)
        {
            index = i;
            break;
        }
    }
    if (index != -1)
    {
        branches.removeAt(index);
    }
}

void ConfigParser::InsertApplication(const QString& branchID, const QString& appID, const AppVersion& version)
{
    if (version.isToolSet)
    {
        for (const QString& fakeAppID : GetTranslatedToolsetApplications())
        {
            InsertApplicationImpl(branchID, fakeAppID, version);
        }
    }
    else
    {
        InsertApplicationImpl(branchID, appID, version);
    }
}

void ConfigParser::InsertApplicationImpl(const QString& branchID, const QString& appID, const AppVersion& version)
{
    Branch* branch = GetBranch(branchID);
    if (!branch)
    {
        branches.push_back(Branch(branchID));
        branch = GetBranch(branchID);
    }

    Application* app = branch->GetApplication(appID);
    if (!app)
    {
        branch->applications.push_back(Application(appID));
        app = branch->GetApplication(appID);
    }

    app->versions.clear();
    app->versions.push_back(version);
    qSort(app->versions.begin(), app->versions.end(), LessThan);
}

void ConfigParser::RemoveApplication(const QString& branchID, const QString& appID, const QString& versionID)
{
    Branch* branch = GetBranch(branchID);
    if (!branch)
        return;

    Application* app = branch->GetApplication(appID);
    if (!app)
        return;

    AppVersion* appVersion = app->GetVersion(versionID);
    if (!appVersion)
        return;

    app->RemoveVersion(versionID);
    if (!app->GetVerionsCount())
        branch->RemoveApplication(appID);

    if (!branch->GetAppCount())
        RemoveBranch(branchID);
}

QStringList ConfigParser::GetToolsetApplications()
{
    static QStringList applications;
    if (applications.isEmpty())
    {
        applications << "AssetCacheServer"
                     << "ResourceEditor"
                     << "QuickEd";
        //try to get project name as it stored in ba-manager
        QString prefix =
#ifdef Q_OS_WIN
        "_win";
#elif defined(Q_OS_MAC)
        "_mac";
#else
#error "unsupported platform"
#endif //platform
        for (auto iter = applications.begin(); iter != applications.end(); ++iter)
        {
            *iter += prefix;
        }
    }
    return applications;
}

QStringList ConfigParser::GetTranslatedToolsetApplications() const
{
    QStringList applications = GetToolsetApplications();
    for (auto iter = applications.begin(); iter != applications.end(); ++iter)
    {
        *iter = GetString(*iter);
    }
    return applications;
}

int ConfigParser::GetBranchCount()
{
    return branches.size();
}

QString ConfigParser::GetBranchID(int branchIndex)
{
    if (branchIndex >= 0 && branchIndex < branches.size())
        return branches[branchIndex].id;

    return QString();
}

Branch* ConfigParser::GetBranch(int branchIndex)
{
    if (branchIndex >= 0 && branchIndex < branches.size())
        return &branches[branchIndex];

    return nullptr;
}

Branch* ConfigParser::GetBranch(const QString& branch)
{
    int branchCount = branches.size();
    for (int i = 0; i < branchCount; ++i)
        if (branches[i].id == branch)
            return &branches[i];

    return nullptr;
}

const Branch* ConfigParser::GetBranch(int branchIndex) const
{
    return const_cast<ConfigParser*>(this)->GetBranch(branchIndex);
}

const Branch* ConfigParser::GetBranch(const QString& branch) const
{
    return const_cast<ConfigParser*>(this)->GetBranch(branch);
}

Application* ConfigParser::GetApplication(const QString& branchID, const QString& appID)
{
    Branch* branch = GetBranch(branchID);
    if (branch)
    {
        int appCount = branch->applications.size();
        for (int i = 0; i < appCount; ++i)
            if (branch->applications[i].id == appID)
            {
                return &branch->applications[i];
            }
    }

    return nullptr;
}

const Application* ConfigParser::GetApplication(const QString& branchID, const QString& appID) const
{
    const Branch* branch = GetBranch(branchID);
    if (branch != nullptr)
    {
        int appCount = branch->applications.size();
        for (int i = 0; i < appCount; ++i)
            if (branch->applications[i].id == appID)
            {
                return &branch->applications[i];
            }
    }

    return nullptr;
}

AppVersion* ConfigParser::GetAppVersion(const QString& branchID, const QString& appID, const QString& ver)
{
    Application* app = GetApplication(branchID, appID);
    if (app != nullptr)
    {
        int versCount = app->versions.size();
        for (int i = 0; i < versCount; ++i)
            if (app->versions[i].id == ver)
                return &app->versions[i];
    }

    return nullptr;
}

AppVersion* ConfigParser::GetAppVersion(const QString& branchID, const QString& appID)
{
    Application* app = GetApplication(branchID, appID);
    if (app != nullptr)
    {
        if (app->versions.isEmpty() == false)
        {
            return &app->versions.first();
        }
    }

    return nullptr;
}

QString ConfigParser::GetString(const QString& stringID) const
{
    if (strings.contains(stringID))
        return strings[stringID];

    return stringID;
}

const QMap<QString, QString>& ConfigParser::GetStrings() const
{
    return strings;
}

const QString& ConfigParser::GetLauncherVersion() const
{
    return launcherVersion;
}

const QString& ConfigParser::GetLauncherURL() const
{
    return launcherURL;
}

const QString& ConfigParser::GetWebpageURL() const
{
    return webPageURL;
}

void ConfigParser::SetLauncherURL(const QString& url)
{
    launcherURL = url;
}

void ConfigParser::SetWebpageURL(const QString& url)
{
    webPageURL = url;
}

void ConfigParser::SetRemoteConfigURL(const QString& url)
{
    remoteConfigURL = url;
}

void ConfigParser::MergeBranchesIDs(QSet<QString>& branchIDs)
{
    int branchCount = branches.size();
    for (int i = 0; i < branchCount; ++i)
        branchIDs.insert(branches[i].id);
}

const QStringList& ConfigParser::GetFavorites()
{
    return favorites;
}

//expected format of input string: 0.8_2015-02-14_11.20.12_0000,
//where 0.8 - DAVA version, 2015-02-14 - build date, 11.20.12 - build time and 0000 - build version
//all blocks can be modified or empty
bool LessThan(const AppVersion& leftVer, const AppVersion& rightVer)
{
    //ignore non-toolset builds
    if (leftVer.isToolSet != rightVer.isToolSet)
    {
        return leftVer.isToolSet == false;
    }

    const QString& leftBuildNum = leftVer.buildNum;
    const QString& rightBuildNum = rightVer.buildNum;

    if (leftBuildNum != rightBuildNum)
    {
        return leftBuildNum < rightBuildNum;
    }

    const QString& left = leftVer.id;
    const QString& right = rightVer.id;

    QStringList leftList = left.split('_', QString::SkipEmptyParts);
    QStringList rightList = right.split('_', QString::SkipEmptyParts);

    int minSize = qMin(leftList.size(), rightList.size());
    for (int i = 0; i < minSize; ++i)
    {
        const QString& leftSubStr = leftList.at(i);
        const QString& rightSubStr = rightList.at(i);
        QStringList leftSubList = leftSubStr.split('.', QString::SkipEmptyParts);
        QStringList rightSubList = rightSubStr.split('.', QString::SkipEmptyParts);
        int subMinSize = qMin(leftSubList.size(), rightSubList.size());
        for (int subStrIndex = 0; subStrIndex < subMinSize; ++subStrIndex)
        {
            bool leftOk;
            bool rightOk;
            const QString& leftSubSubStr = leftSubList.at(subStrIndex);
            const QString& rightSubSubStr = rightSubList.at(subStrIndex);
            qlonglong leftVal = leftSubSubStr.toLongLong(&leftOk);
            qlonglong rightVal = rightSubSubStr.toLongLong(&rightOk);
            if (leftOk && rightOk)
            {
                if (leftVal != rightVal)
                {
                    return leftVal < rightVal;
                }
            }
            else //date format or other
            {
                if (leftSubSubStr != rightSubSubStr)
                {
                    return leftSubSubStr < rightSubSubStr;
                }
            }
        }
        //if version lists are equal - checking for extra subversion
        if (leftSubList.size() != rightSubList.size())
        {
            return leftSubList.size() < rightSubList.size();
        }
    }
    return false; // string are equal
};
