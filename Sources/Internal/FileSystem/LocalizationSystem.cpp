#include "FileSystem/LocalizationSystem.h"

#include "Debug/DVAssert.h"
#include "Engine/Engine.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/KeyedArchive.h"
#include "FileSystem/YamlEmitter.h"
#include "FileSystem/YamlNode.h"
#include "Logger/Logger.h"
#include "Platform/DeviceInfo.h"
#include "Sound/SoundSystem.h"
#include "Utils/StringFormat.h"
#include "Utils/UTF8Utils.h"
#include "Utils/Utils.h"

#define YAML_DECLARE_STATIC
#include "yaml/yaml.h"

#if defined(__DAVAENGINE_ANDROID__)
#include "Engine/PlatformApiAndroid.h"
#endif

namespace DAVA
{
//TODO: move it to DateTimeWin32 or remove
const Vector<LocalizationSystem::LanguageLocalePair> LocalizationSystem::languageLocaleMap =
{
  { "en", "en_US" },
  { "ru", "ru_RU" },
  { "de", "de_DE" },
  { "it", "it_IT" },
  { "fr", "fr_FR" },
  { "es", "es_ES" },
  { "zh", "zh_CN" },
  { "ja", "ja_JP" },
  { "uk", "uk_UA" }
};

const char* LocalizationSystem::DEFAULT_LOCALE = "en";

const KeyedArchive* GetOptions()
{
    return Engine::Instance()->GetOptions();
}

LocalizationSystem::LocalizationSystem()
{
    langId = DEFAULT_LOCALE;

    dataHolder = new YamlParser::YamlDataHolder();
    dataHolder->data = 0;
}

LocalizationSystem::~LocalizationSystem()
{
    Cleanup();
    SafeDelete(dataHolder);
}

void LocalizationSystem::InitWithDirectory(const FilePath& directoryPath)
{
    SetDirectory(directoryPath);
    Init();
}

void LocalizationSystem::SetDirectory(const FilePath& dirPath)
{
    DVASSERT(dirPath.IsDirectoryPathname());
    directoryPath = dirPath;
    String locale = GetDeviceLocale();

    if (locale.empty())
    {
        DVASSERT(false, "GetDeviceInfo() is not implemented for current platform! Used default locale!");
        locale = GetOptions()->GetString("locale", DEFAULT_LOCALE);
    }
    SetCurrentLocale(locale);
}

#if !defined(__DAVAENGINE_ANDROID__)

String LocalizationSystem::GetDeviceLocale(void) const
{
    if (!overridenLangId.empty())
    {
        return overridenLangId;
    }

    String locale = DeviceInfo::GetLocale();
    String::size_type posEnd = locale.find('-', 2);
    if (String::npos != posEnd)
    {
        locale = locale.substr(0, posEnd);
    }
    return locale;
}

#else
String LocalizationSystem::GetDeviceLocale(void) const
{
    if (!overridenLangId.empty())
    {
        return overridenLangId;
    }

    JNI::JavaClass jniLocalisation("com/dava/engine/Localization");
    Function<jstring()> jgetLocale = jniLocalisation.GetStaticMethod<jstring>("GetLocale");

    return JNI::JavaStringToString(JNI::LocalRef<jstring>(jgetLocale()));
}
#endif

void LocalizationSystem::Init()
{
    LoadStringFile(langId, directoryPath + (langId + ".yaml"));
}

const String& LocalizationSystem::GetCurrentLocale() const
{
    return langId;
}

const FilePath& LocalizationSystem::GetDirectoryPath() const
{
    return directoryPath;
}

void LocalizationSystem::OverrideDeviceLocale(const String& langId)
{
    overridenLangId = langId;
}

bool LocalizationSystem::SetCurrentLocale(const String& requestedLangId)
{
    bool requestedLocaleFound = true;

    String actualLangId;

    FilePath localeFilePath(directoryPath + (requestedLangId + ".yaml"));
    if (FileSystem::Instance()->Exists(localeFilePath))
    {
        actualLangId = requestedLangId;
    }
    else if (requestedLangId.size() > 2)
    {
        String langPart = requestedLangId.substr(0, 2);
        String::size_type posPartStart = 3;
        // ex. zh-Hans, zh-Hans-CN, zh-Hans_CN, zh_Hans_CN, zh_CN, zh

        String::size_type posScriptEnd = requestedLangId.find('-', posPartStart);
        if (posScriptEnd == String::npos)
        {
            // ex. not zh-Hans-CN, but can be zh-Hans_CN
            posScriptEnd = requestedLangId.find('_', posPartStart);
        }

        String scriptPart = requestedLangId.substr(posPartStart);
        if (posScriptEnd != String::npos)
        {
            // ex. zh-Hans-CN or zh-Hans_CN try zh-Hans
            scriptPart = requestedLangId.substr(posPartStart, posScriptEnd - posPartStart);
        }
        // ex. zh_CN, zh-HK
        if (scriptPart == "CN" || (langPart == "zh" && scriptPart == ""))
        {
            scriptPart = "Hans";
        }
        else if (scriptPart == "TW" || scriptPart == "HK")
        {
            scriptPart = "Hant";
        }
        langPart = Format("%s-%s", langPart.c_str(), scriptPart.c_str());

        Logger::FrameworkDebug("LocalizationSystem requested locale %s is not supported, trying to check part %s", requestedLangId.c_str(), langPart.c_str());
        localeFilePath = directoryPath + (langPart + ".yaml");
        if (FileSystem::Instance()->Exists(localeFilePath))
        {
            actualLangId = langPart;
        }
        else if (langPart == "zh")
        {
            // in case zh is returned without country code and no zh.yaml is found - try zh-Hans
            langPart = "zh-Hans";
            localeFilePath = directoryPath + (langPart + ".yaml");
            if (localeFilePath.Exists())
            {
                actualLangId = langPart;
            }
        }
    }

    if (actualLangId.empty())
    {
        requestedLocaleFound = false;

        localeFilePath = directoryPath + (String(DEFAULT_LOCALE) + ".yaml");
        if (FileSystem::Instance()->Exists(localeFilePath))
        {
            actualLangId = DEFAULT_LOCALE;
        }
        else
        {
            Logger::Warning("LocalizationSystem requested locale %s is not supported, failed to set default lang, locale will not be changed", requestedLangId.c_str(), actualLangId.c_str());
            return requestedLocaleFound;
        }
    }

    //TODO: add reloading strings data on langId changing
    Logger::FrameworkDebug("LocalizationSystem requested locale: %s, set locale: %s", requestedLangId.c_str(), actualLangId.c_str());
    langId = actualLangId;
    SoundSystem::Instance()->SetCurrentLocale(langId);
    return requestedLocaleFound;
}

LocalizationSystem::StringFile* LocalizationSystem::LoadFromYamlFile(const String& langID, const FilePath& pathName)
{
    yaml_parser_t parser;
    yaml_event_t event;

    int done = 0;

    /* Create the Parser object. */
    yaml_parser_initialize(&parser);

    yaml_parser_set_encoding(&parser, YAML_UTF8_ENCODING);

    File* yamlFile = File::Create(pathName, File::OPEN | File::READ);
    if (!yamlFile)
        return NULL;

    dataHolder->fileSize = static_cast<uint32>(yamlFile->GetSize());
    dataHolder->data = new uint8[dataHolder->fileSize];
    dataHolder->dataOffset = 0;
    yamlFile->Read(dataHolder->data, dataHolder->fileSize);
    yamlFile->Release();

    yaml_parser_set_input(&parser, read_handler, dataHolder);

    String utf8Key;
    bool isKey = true;
    StringFile* strFile = new StringFile();

    /* Read the event sequence. */
    while (!done)
    {
        /* Get the next event. */
        if (!yaml_parser_parse(&parser, &event))
        {
            Logger::Error("parsing error: type: %d %s line: %d pos: %d", parser.error, parser.problem, parser.problem_mark.line, parser.problem_mark.column);
            SafeDelete(strFile);
            break;
        }

        switch (event.type)
        {
        case YAML_ALIAS_EVENT:
            //Logger::FrameworkDebug("alias: %s", event.data.alias.anchor);
            break;

        case YAML_SCALAR_EVENT:
        {
            const auto& scalar = event.data.scalar;
            String utf8String(scalar.value, scalar.value + scalar.length);
            if (isKey)
            {
                utf8Key = utf8String;
            }
            else
            {
                strFile->strings[utf8Key] = utf8String;
            }

            isKey = !isKey;
        }
        break;

        case YAML_DOCUMENT_START_EVENT:
        {
            //Logger::FrameworkDebug("document start:");
        }
        break;

        case YAML_DOCUMENT_END_EVENT:
        { //Logger::FrameworkDebug("document end:");
        }
        break;

        case YAML_SEQUENCE_START_EVENT:
        {
        }
        break;

        case YAML_SEQUENCE_END_EVENT:
        {
        }
        break;

        case YAML_MAPPING_START_EVENT:
        {
        }
        break;

        case YAML_MAPPING_END_EVENT:
        {
        }
        break;
        default:
            break;
        };

        /* Are we finished? */
        done = (event.type == YAML_STREAM_END_EVENT);

        /* The application is responsible for destroying the event object. */
        yaml_event_delete(&event);
    }

    yaml_parser_delete(&parser);
    if (strFile)
    {
        strFile->pathName = pathName;
        strFile->langId = langID;
    }

    SafeDeleteArray(dataHolder->data);
    return strFile;
}

bool LocalizationSystem::SaveToYamlFile(const StringFile* stringFile)
{
    if (!stringFile)
    {
        return false;
    }

    RefPtr<YamlNode> node = YamlNode::CreateMapNode(true, YamlNode::MR_BLOCK_REPRESENTATION, YamlNode::SR_DOUBLE_QUOTED_REPRESENTATION);
    for (auto iter = stringFile->strings.begin(); iter != stringFile->strings.end(); ++iter)
    {
        node->Add(iter->first, iter->second);
    }

    bool result = YamlEmitter::SaveToYamlFile(stringFile->pathName, node.Get());

    return result;
}

void LocalizationSystem::LoadStringFile(const String& langID, const FilePath& fileName)
{
    StringFile* file = LoadFromYamlFile(langID, fileName);
    if (file)
    {
        stringsList.push_back(file);
    }
}

void LocalizationSystem::UnloadStringFile(const FilePath& fileName)
{
    DVASSERT(0 && "Method do not implemented");
}

String LocalizationSystem::GetLocalizedString(const String& utf8Key) const
{
    for (auto it = stringsList.rbegin(); it != stringsList.rend(); ++it)
    {
        StringFile* file = *it;

        auto res = file->strings.find(utf8Key);
        if (res != file->strings.end())
        {
            return res->second;
        }
    }
    return utf8Key;
}

String LocalizationSystem::GetLocalizedString(const String& utf8Key, const String& langId) const
{
    for (auto it = stringsList.rbegin(); it != stringsList.rend(); ++it)
    {
        StringFile* file = *it;

        if (file->langId.compare(langId) == 0)
        {
            auto res = file->strings.find(utf8Key);
            if (res != file->strings.end())
            {
                return res->second;
            }
        }
    }
    return utf8Key;
}

void LocalizationSystem::SetLocalizedString(const String& utf8Key, const String& utf8Value)
{
    // Update in all files currently loaded.
    for (auto it = stringsList.rbegin(); it != stringsList.rend(); ++it)
    {
        StringFile* file = *it;
        file->strings[utf8Key] = utf8Value;
    }
}

void LocalizationSystem::RemoveLocalizedString(const String& utf8Key)
{
    // Update in all files currently loaded.
    for (auto it = stringsList.rbegin(); it != stringsList.rend(); ++it)
    {
        StringFile* file = *it;
        file->strings.erase(utf8Key);
    }
}

bool LocalizationSystem::SaveLocalizedStrings()
{
    bool saveResult = true;
    for (auto it = stringsList.rbegin(); it != stringsList.rend(); ++it)
    {
        StringFile* file = *it;
        saveResult &= SaveToYamlFile(file);
    }

    return saveResult;
}

void LocalizationSystem::Cleanup()
{
    // release all memory allocated by strings
    for (auto it = stringsList.rbegin(); it != stringsList.rend(); ++it)
    {
        StringFile* file = *it;
        SafeDelete(file);
    }
    stringsList.clear();

    directoryPath = FilePath();
    langId.clear();
    SafeDeleteArray(dataHolder->data);
}

bool LocalizationSystem::GetStringsForCurrentLocale(Map<String, String>& utf8Strings) const
{
    for (auto iter = stringsList.begin(); iter != stringsList.end();
         ++iter)
    {
        if ((*iter)->langId == GetCurrentLocale())
        {
            utf8Strings = (*iter)->strings;
            return true;
        }
    }

    // No strings found.
    return false;
}

String LocalizationSystem::GetCountryCode() const
{
    auto iter = std::find_if(languageLocaleMap.begin(), languageLocaleMap.end(), [&](const LocalizationSystem::LanguageLocalePair& langPair)
                             {
                                 return langPair.languageCode == langId;
                             });

    if (iter != languageLocaleMap.end())
    {
        return (*iter).localeCode;
    }

    return "en_US";
}

WideString LocalizedWideString(const String& utf8Key)
{
    return UTF8Utils::EncodeToWideString(LocalizationSystem::Instance()->GetLocalizedString(utf8Key));
}
};
