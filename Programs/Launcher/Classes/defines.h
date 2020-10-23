#ifndef DEFINES_H
#define DEFINES_H

#include <QtGlobal>

////Global launcher defines

#define LAUNCHER_VER __DATE__

#define LOCAL_CONFIG_NAME "localConfig.yaml"

////YAML config keys

#define CONFIG_STRINGS_KEY "strings"
#define CONFIG_LAUNCHER_KEY "launcher"
#define CONFIG_BRANCHES_KEY "branches"

#define CONFIG_LAUNCHER_VERSION_KEY "version"
#define CONFIG_LAUNCHER_WEBPAGE_KEY "News"
#define CONFIG_LAUNCHER_NEWSID_KEY "newsID"
#define CONFIG_LAUNCHER_FAVORITES_KEY "favorites"

#define CONFIG_APPVERSION_RUNPATH_KEY "runpath"
#define CONFIG_APPVERSION_CMD_KEY "cmd"

#define CONFIG_URL_KEY "url"

////Application table defines

#define COLUMN_APP_NAME 0
#define COLUMN_APP_VERSION 1
#define COLUMN_BUTTONS 2

#define DAVA_CUSTOM_PROPERTY_NAME "DAVA_ID"

#ifdef Q_OS_WIN
#define TABLE_STYLESHEET "QComboBox {margin-top: 7px; margin-bottom: 7px; padding-left: 5px;} QLabel {padding-left: 4px;padding-right: 4px;}"
#elif defined(Q_OS_MAC)
#define TABLE_STYLESHEET "QLabel {padding-left: 4px;padding-right: 4px;}"
#endif

////Update dialog defines

#define LOG_COLOR_COMPLETE QColor(0, 110, 0)
#define LOG_COLOR_PROGRESS QColor(0, 0, 110)
#define LOG_COLOR_FAIL QColor(240, 0, 0)

#ifdef Q_OS_WIN
#define platformString QString("windows")
#elif defined Q_OS_MAC
#define platformString QString("macos")
#endif //platform

/////////////////////

template <class TYPE>
void SafeDelete(TYPE*& d)
{
    if (d)
    {
        delete d;
        d = 0;
    }
}

#endif // DEFINES_H
