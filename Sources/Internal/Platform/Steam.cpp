#include "Steam.h"

#if defined(__DAVAENGINE_STEAM__)

#include "Engine/Engine.h"
#include "Logger/Logger.h"
#include "FileSystem/KeyedArchive.h"
#include "FileSystem/LocalizationSystem.h"

#include "steam/steam_api.h"

#include <cstdlib>

namespace DAVA
{
struct SteamCallbacks
{
    STEAM_CALLBACK(SteamCallbacks, GameOverlayActivated, GameOverlayActivated_t)
    {
        Steam::GameOverlayActivated.Emit(pParam->m_bActive != 0);
    }
    // Place other Steam callbacks here
};
static SteamCallbacks* steamCallbacks = nullptr;

const String Steam::appIdPropertyKey = "steam_appid";
bool Steam::isInited = false;
Signal<bool> Steam::GameOverlayActivated;

void Steam::Init()
{
    uint32 useAppId = Engine::Instance()->GetOptions()->GetUInt32(appIdPropertyKey, k_uAppIdInvalid);
    if (SteamAPI_RestartAppIfNecessary(useAppId))
    {
        // if Steam is not running or the game wasn't started through Steam, SteamAPI_RestartAppIfNecessary starts the
        // local Steam client and also launches this game again.

        // Once you get a public Steam AppID assigned for this game, you need to replace k_uAppIdInvalid with it and
        // removed steam_appid.txt from the game depot.
        Logger::Error("Error SteamAPI Restart.");
        std::exit(0);
    }

    // Initialize SteamAPI, if this fails we bail out since we depend on Steam for lots of stuff.
    // You don't necessarily have to though if you write your code to check whether all the Steam
    // interfaces are NULL before using them and provide alternate paths when they are unavailable.
    //
    // This will also load the in-game steam overlay dll into your process.  That dll is normally
    // injected by steam when it launches games, but by calling this you cause it to always load,
    // even when not launched via steam.
    if (!SteamAPI_Init())
    {
        Logger::Error("SteamAPI_Init() failed\n Fatal Error: Steam must be running to play this game(SteamAPI_Init() failed).");
        return;
    }

    if (!SteamController()->Init())
    {
        Logger::Error("SteamController()->Init failed.\n Fatal Error: SteamController()->Init failed.");
        return;
    }

    isInited = true;
    steamCallbacks = new SteamCallbacks();
    GetEngineContext()->localizationSystem->OverrideDeviceLocale(GetLanguage());
    Engine::Instance()->update.Connect([](float) { SteamAPI_RunCallbacks(); });
}

void Steam::Deinit()
{
    SafeDelete(steamCallbacks);

    // Shutdown the SteamAPI
    SteamAPI_Shutdown();
    isInited = false;
}

bool Steam::IsInited()
{
    return isInited;
}

String Steam::GetLanguage()
{
    if (!IsInited())
    {
        return "";
    }

    ISteamApps* apps = SteamApps();
    if (apps == nullptr)
    {
        return "";
    }

    // Try to get a set language for game
    String language = apps->GetCurrentGameLanguage();
    if (language.empty())
    {
        // If it fails, use steam app language
        ISteamUtils* steamUtils = SteamUtils();
        language = steamUtils != nullptr ? steamUtils->GetSteamUILanguage() : "";
    }

    const UnorderedMap<String, String> steamLanguages =
    {
      { "arabic", "ar" },
      { "brazilian", "pt" },
      { "bulgarian", "bg" },
      { "czech", "cs" },
      { "danish", "da" },
      { "dutch", "nl" },
      { "english", "en" },
      { "finnish", "fi" },
      { "french", "fr" },
      { "german", "de" },
      { "greek", "el" },
      { "hungarian", "hu" },
      { "italian", "it" },
      { "japanese", "ja" },
      { "koreana", "ko" },
      { "norwegian", "no" },
      { "polish", "pl" },
      { "portuguese", "pt" },
      { "romanian", "ro" },
      { "russian", "ru" },
      { "schinese", "zh-Hans" },
      { "spanish", "es" },
      { "swedish", "sv" },
      { "tchinese", "zh-Hant" },
      { "thai", "th" },
      { "turkish", "tr" },
      { "ukrainian", "uk" },
      { "vietnamese", "vi" }
    };

    auto iter = steamLanguages.find(language);
    return iter != steamLanguages.end() ? iter->second : "";
}

ISteamRemoteStorage* Steam::CreateStorage()
{
    return SteamRemoteStorage();
}
}

#endif
