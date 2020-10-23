#pragma once

#include "Data/ConfigParser.h"
#include <QString>
#include <memory>

struct ApplicationContext;
struct ConfigHolder;

class ConfigParser;
struct Branch;
struct Application;
struct AppVersion;

namespace LauncherUtils
{
QString GetAppName(const QString& appName, bool isToolSet);
QString GetLocalAppPath(const AppVersion* version, const QString& appID);
QString GetApplicationDirectory(const ConfigHolder* configHolder, const ApplicationContext* context, QString branchID, QString appID, bool isToolSet);
bool CanTryStopApplication(const QString& applicationName);

Branch* FindBranch(ConfigParser* config, const QString& branchName);
Application* FindApplication(Branch* branch, QString appName);
Application* FindApplication(ConfigParser* config, const QString& branchName, const QString& appName);
AppVersion* FindVersion(Application* app, QString versionName);
AppVersion* FindVersion(Branch* branch, const QString& appName, const QString& versionName);
AppVersion* FindVersion(ConfigParser* config, const QString& branchName, const QString& appName, const QString& versionName);
}
