#pragma once

#include "Base/BaseTypes.h"
#include "Base/Singleton.h"

#include "FileSystem/YamlParser.h"

namespace DAVA
{
class LocalizationSystem : public Singleton<LocalizationSystem>
{
public:
    static const char* DEFAULT_LOCALE;

    LocalizationSystem();
    virtual ~LocalizationSystem();

    void InitWithDirectory(const FilePath& directoryPath);

    void Init();

    const String& GetCurrentLocale() const;
    /** Set locale. If strings file not found return false. */
    bool SetCurrentLocale(const String& newLangId);
    void OverrideDeviceLocale(const String& langId);
    String GetDeviceLocale() const;

    String GetCountryCode() const;

    String GetLocalizedString(const String& utf8Key) const;
    String GetLocalizedString(const String& utf8Key, const String& langId) const;
    void SetLocalizedString(const String& utf8Key, const String& utf8Value);
    void RemoveLocalizedString(const String& utf8Key);

    void SetDirectory(const FilePath& dirPath);
    const FilePath& GetDirectoryPath() const;

    void Cleanup();

    // Access to the whole strings list for the current locale.
    // Returns FALSE if no strings found.
    bool GetStringsForCurrentLocale(Map<String, String>& utf8Strings) const;

    // Save the current localization data to the files they were loaded from.
    bool SaveLocalizedStrings();

private:
    struct LanguageLocalePair
    {
        String languageCode; // in ISO 639-1, like en,ru,uk
        String localeCode; // like en_US, ru_RU
    };
    static const Vector<LanguageLocalePair> languageLocaleMap;

    void LoadStringFile(const String& langId, const FilePath& fileName);
    void UnloadStringFile(const FilePath& fileName);

    String langId;
    String overridenLangId;
    FilePath directoryPath;

    struct StringFile
    {
        FilePath pathName;
        String langId;
        Map<String, String> strings;
    };
    List<StringFile*> stringsList;

    // Load/Save functionality.
    StringFile* LoadFromYamlFile(const String& langID, const FilePath& fileName);
    bool SaveToYamlFile(const StringFile* stringFile);

    YamlParser::YamlDataHolder* dataHolder;
};

inline String LocalizedUtf8String(const String& utf8Key)
{
    return LocalizationSystem::Instance()->GetLocalizedString(utf8Key);
}

WideString LocalizedWideString(const String& utf8Key);
};
