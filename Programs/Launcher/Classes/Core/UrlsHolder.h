#pragma once

#include <vector>
#include <array>
#include <memory>

#include <QString>
#include <QUrl>

class BaseTask;

class UrlsHolder
{
public:
    //word URL added to resolve name conflict
    //enum must be started with zero to make loop through it
    enum eURLType
    {
        LauncherInfoURL = 0,
        LauncherTestInfoURL,
        StringsURL,
        FavoritesURL,
        AllBuildsCurrentPlatformURL,
        AllBuildsAndroidURL,
        AllBuildsIOSURL,
        AllBuildsUWPURL,
        URLTypesCount
    };

    UrlsHolder();

    QString GetURL(eURLType type) const;
    QString GetServerHostName() const;
    void SetServerHostName(const QString& url);

    bool IsTestAPIUsed() const;
    void SetUseTestAPI(bool use);

    std::vector<QUrl> GetURLs() const;

private:
    bool useTestAPI = false;

    std::array<QString, URLTypesCount> urls;
    QString serverHostName;
};
