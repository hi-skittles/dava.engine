#include "EngineSettings_impl.h"
#include "Engine/Engine.h"
#include "Engine/EngineSettings.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/YamlParser.h"
#include "FileSystem/YamlNode.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
struct EngineSettings::EngineSettingsDetails
{
    template <eSetting ID, typename T>
    static void SetupSetting(ReflectionRegistrator<EngineSettings>& registrator, const char* name, const T& defaultValue = T(), const T& rangeStart = T(), const T& rangeEnd = T());
    static void SetupSettingValue(eSettingValue value, const char* name);

    static std::array<Any, SETTING_COUNT> settingDefault;
    static std::array<FastName, SETTING_COUNT> settingName;
    static std::array<FastName, SETTING_VALUE_COUNT> settingValueName;
};

std::array<Any, EngineSettings::SETTING_COUNT> EngineSettings::EngineSettingsDetails::settingDefault;
std::array<FastName, EngineSettings::SETTING_COUNT> EngineSettings::EngineSettingsDetails::settingName;
std::array<FastName, EngineSettings::SETTING_VALUE_COUNT> EngineSettings::EngineSettingsDetails::settingValueName;

DAVA_REFLECTION_IMPL(EngineSettings)
{
    auto registrator = ReflectionRegistrator<EngineSettings>::Begin();

    //settings setup
    EngineSettingsDetails::SetupSetting<SETTING_LANDSCAPE_RENDERMODE, eSettingValue>(registrator, "Landscape.RenderMode", LANDSCAPE_MORPHING, LANDSCAPE_NO_INSTANCING, LANDSCAPE_MORPHING);
    EngineSettingsDetails::SetupSetting<SETTING_PROFILE_DLC_MANAGER, bool>(registrator, "DlcManagerProfiling");

    //setting enum values setup
    EngineSettingsDetails::SetupSettingValue(LANDSCAPE_NO_INSTANCING, "Landscape.RenderMode.NoInstancing");
    EngineSettingsDetails::SetupSettingValue(LANDSCAPE_INSTANCING, "Landscape.RenderMode.Instancing");
    EngineSettingsDetails::SetupSettingValue(LANDSCAPE_MORPHING, "Landscape.RenderMode.Morphing");

    registrator.End();
}

EngineSettings::EngineSettings()
{
    ReflectedTypeDB::Get<EngineSettings>(); //ensure that settings was setup

    Reset();
}

void EngineSettings::Reset()
{
    for (int32 i = 0; i < int32(SETTING_COUNT); ++i)
        setting[i] = EngineSettingsDetails::settingDefault[i];
}

bool EngineSettings::Load(const FilePath& filepath)
{
    if (GetEngineContext()->fileSystem->Exists(filepath))
    {
        RefPtr<YamlParser> parser(YamlParser::Create(filepath));
        YamlNode* rootNode = parser->GetRootNode();

        if (rootNode)
        {
            for (uint32 i = 0; i < SETTING_COUNT; ++i)
            {
                const FastName& settingName = GetSettingName(eSetting(i));
                const YamlNode* settingNode = rootNode->Get(settingName.c_str());
                if (settingNode && settingNode->GetType() == YamlNode::TYPE_STRING)
                {
                    if (setting[i].GetType() == Type::Instance<bool>())
                        setting[i] = settingNode->AsBool();
                    else if (setting[i].GetType() == Type::Instance<int32>())
                        setting[i] = settingNode->AsInt32();
                    else if (setting[i].GetType() == Type::Instance<float32>())
                        setting[i] = settingNode->AsFloat();
                    else if (setting[i].GetType() == Type::Instance<String>())
                        setting[i] = settingNode->AsString();
                    else if (setting[i].GetType() == Type::Instance<eSettingValue>())
                        setting[i] = GetSettingValueByName(FastName(settingNode->AsString().c_str()));
                }
            }

            return true;
        }
    }

    return false;
}

const FastName& EngineSettings::GetSettingName(eSetting setting)
{
    DVASSERT(setting < SETTING_COUNT);
    return EngineSettingsDetails::settingName[setting];
}

const FastName& EngineSettings::GetSettingValueName(eSettingValue value)
{
    DVASSERT(value < SETTING_VALUE_COUNT);
    return EngineSettingsDetails::settingValueName[value];
}

EngineSettings::eSettingValue EngineSettings::GetSettingValueByName(const FastName& name)
{
    uint32 i = 0;
    for (; i < SETTING_VALUE_COUNT; ++i)
    {
        if (EngineSettingsDetails::settingValueName[i] == name)
            break;
    }

    return eSettingValue(i);
}

//Details definition
template <EngineSettings::eSetting ID, typename T>
void EngineSettings::EngineSettingsDetails::SetupSetting(ReflectionRegistrator<EngineSettings>& registrator, const char* name, const T& defaultValue, const T& rangeStart, const T& rangeEnd)
{
    DVASSERT(rangeStart <= rangeEnd);

    settingDefault[ID] = defaultValue;
    settingName[ID] = FastName(name);

    if (rangeStart != rangeEnd)
    {
        registrator.Field(GetSettingName(ID).c_str(), &EngineSettings::GetSettingRefl<ID, T>, &EngineSettings::SetSettingRefl<ID, T>)[Meta<SettingRange<T>>(rangeStart, rangeEnd)];
    }
    else
    {
        registrator.Field(GetSettingName(ID).c_str(), &EngineSettings::GetSettingRefl<ID, T>, &EngineSettings::SetSettingRefl<ID, T>);
    }
}

void EngineSettings::EngineSettingsDetails::SetupSettingValue(eSettingValue value, const char* name)
{
    DVASSERT(value < EngineSettings::SETTING_VALUE_COUNT);
    settingValueName[value] = FastName(name);
}

} // end namespace DAVA
