#include "Core/UrlsHolder.h"
#include "Utils/ErrorMessenger.h"

UrlsHolder::UrlsHolder()
    : serverHostName("http://ba-manager.wargaming.net")
{
}

QString UrlsHolder::GetURL(eURLType type) const
{
    switch (type)
    {
    case LauncherInfoURL:
        return "/modules/jsonAPI/launcher/lite.php?source=launcher";
    case LauncherTestInfoURL:
        return "/modules/jsonAPI/launcher/lite4test.php?source=launcher";
    case StringsURL:
        return "/modules/jsonAPI/launcher/lite.php?source=seo_list";
    case FavoritesURL:
        return "/modules/jsonAPI/launcher/lite.php?source=branches&filter=os:" + platformString;
    case AllBuildsCurrentPlatformURL:
        return "/modules/jsonAPI/launcher/lite.php?source=builds&filter=os:" + platformString;
    case AllBuildsAndroidURL:
        return "/modules/jsonAPI/launcher/lite.php?source=builds&filter=os:android";
    case AllBuildsIOSURL:
        return "/modules/jsonAPI/launcher/lite.php?source=builds&filter=os:ios";
    case AllBuildsUWPURL:
        return "/modules/jsonAPI/launcher/lite.php?source=builds&filter=os:uwp";
    default:
        Q_ASSERT(false && "unacceptable request");
        return QString();
    }
}

QString UrlsHolder::GetServerHostName() const
{
    return serverHostName;
}

bool UrlsHolder::IsTestAPIUsed() const
{
    return useTestAPI;
}

void UrlsHolder::SetUseTestAPI(bool use)
{
    useTestAPI = use;
}

std::vector<QUrl> UrlsHolder::GetURLs() const
{
    std::vector<QUrl> urls;
    if (IsTestAPIUsed())
    {
        urls.push_back(serverHostName + GetURL(LauncherTestInfoURL));
    }
    else
    {
        urls.push_back(serverHostName + GetURL(LauncherInfoURL));
    }

    for (int i = StringsURL; i < URLTypesCount; ++i)
    {
        eURLType type = static_cast<eURLType>(i);
        QUrl url(serverHostName + GetURL(type));
        urls.push_back(url);
    }
    return urls;
}

void UrlsHolder::SetServerHostName(const QString& url)
{
    serverHostName = url;
}
