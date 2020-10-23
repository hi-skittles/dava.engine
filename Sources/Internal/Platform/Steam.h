#pragma once

#if defined(__DAVAENGINE_STEAM__)
#include "Base/BaseTypes.h"
#include "Functional/Signal.h"

class ISteamRemoteStorage;
namespace DAVA
{
class Steam final
{
public:
    static const String appIdPropertyKey;

    Steam() = default;

    static void Init();
    static void Deinit();
    static bool IsInited();

    /** 
    Return language code from Steam (for example: "ru" for russian, "en" for english).
    Try to return set language for app. 
    If fails, return language of Steam app or empty string if language is unsupported.
    */
    static String GetLanguage();

    static ISteamRemoteStorage* CreateStorage();

    static Signal<bool> GameOverlayActivated;

private:
    static bool isInited;
};
}
#endif
