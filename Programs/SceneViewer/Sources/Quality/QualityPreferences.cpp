#include "QualityPreferences.h"

#include <Scene3D/Systems/QualitySettingsSystem.h>
#include <FileSystem/KeyedArchive.h>

namespace QualityPreferences
{
DAVA::String SETTING_QUALITY_TEXTURE = "Quality/Texture";
DAVA::String SETTING_QUALITY_ANISOTROPY = "Quality/Anisotropy";
DAVA::String SETTING_QUALITY_MSAA = "Quality/MSAA";
DAVA::String SETTING_QUALITY_MATERIALS = "Quality/Material/";
DAVA::String SETTING_QUALITY_PARTICLE = "Quality/Particle";
DAVA::String SETTING_QUALITY_OPTIONS = "Quality/Options/";

void LoadFromSettings(Settings& appSettings)
{
    using namespace DAVA;

    const KeyedArchive* settings = appSettings.GetQualitySettings();
    if (settings != nullptr)
    {
        VariantType* value = settings->GetVariant(SETTING_QUALITY_TEXTURE);
        if (value != nullptr && value->GetType() == VariantType::TYPE_FASTNAME)
        {
            if (QualitySettingsSystem::Instance()->GetTxQuality(value->AsFastName()) != nullptr)
            {
                QualitySettingsSystem::Instance()->SetCurTextureQuality(value->AsFastName());
            }
        }

        value = settings->GetVariant(SETTING_QUALITY_ANISOTROPY);
        if (value != nullptr && value->GetType() == VariantType::TYPE_FASTNAME)
        {
            if (QualitySettingsSystem::Instance()->GetAnisotropyQuality(value->AsFastName()) != nullptr)
            {
                QualitySettingsSystem::Instance()->SetCurAnisotropyQuality(value->AsFastName());
            }
        }

        value = settings->GetVariant(SETTING_QUALITY_MSAA);
        if (value != nullptr && value->GetType() == VariantType::TYPE_FASTNAME)
        {
            if (QualitySettingsSystem::Instance()->GetMSAAQuality(value->AsFastName()) != nullptr)
            {
                QualitySettingsSystem::Instance()->SetCurMSAAQuality(value->AsFastName());
            }
        }

        value = settings->GetVariant(SETTING_QUALITY_MATERIALS);
        if (value != nullptr && value->GetType() == VariantType::TYPE_KEYED_ARCHIVE)
        {
            KeyedArchive* materialQualities = value->AsKeyedArchive();
            KeyedArchive::UnderlyingMap data = materialQualities->GetArchieveData();
            for (KeyedArchive::UnderlyingMap::value_type& entry : data)
            {
                FastName storedGroupName(entry.first);

                size_t count = QualitySettingsSystem::Instance()->GetMaterialQualityGroupCount();
                for (size_t i = 0; i < count; ++i)
                {
                    FastName groupName = QualitySettingsSystem::Instance()->GetMaterialQualityGroupName(i);
                    if (groupName == storedGroupName)
                    {
                        VariantType* value = entry.second;
                        if (value->GetType() == VariantType::TYPE_FASTNAME)
                        {
                            if (QualitySettingsSystem::Instance()->GetMaterialQuality(groupName, value->AsFastName()) != nullptr)
                            {
                                QualitySettingsSystem::Instance()->SetCurMaterialQuality(groupName, value->AsFastName());
                            }
                        }
                    }
                }
            }
        }

        value = settings->GetVariant(SETTING_QUALITY_PARTICLE);
        if (value != nullptr && value->GetType() == VariantType::TYPE_FASTNAME)
        {
            ParticlesQualitySettings& particlesSettings = QualitySettingsSystem::Instance()->GetParticlesQualitySettings();
            if (particlesSettings.GetQualityIndex(value->AsFastName()) != -1)
            {
                particlesSettings.SetCurrentQuality(value->AsFastName());
            }
        }

        value = settings->GetVariant(SETTING_QUALITY_OPTIONS);
        if (value != nullptr && value->GetType() == VariantType::TYPE_KEYED_ARCHIVE)
        {
            KeyedArchive* options = value->AsKeyedArchive();
            KeyedArchive::UnderlyingMap data = options->GetArchieveData();
            for (KeyedArchive::UnderlyingMap::value_type& entry : data)
            {
                FastName storedOptionName(entry.first);
                VariantType* value = entry.second;

                if (value->GetType() == VariantType::TYPE_BOOLEAN)
                {
                    for (int32 i = 0; i < QualitySettingsSystem::Instance()->GetOptionsCount(); ++i)
                    {
                        if (QualitySettingsSystem::Instance()->GetOptionName(i) == storedOptionName)
                        {
                            QualitySettingsSystem::Instance()->EnableOption(storedOptionName, value->AsBool());
                            break;
                        }
                    }
                }
            }
        }
    }
}

void SaveToSettings(Settings& appSettings)
{
    using namespace DAVA;

    ScopedPtr<KeyedArchive> archive(new KeyedArchive);

    archive->SetFastName(SETTING_QUALITY_TEXTURE, QualitySettingsSystem::Instance()->GetCurTextureQuality());
    archive->SetFastName(SETTING_QUALITY_ANISOTROPY, QualitySettingsSystem::Instance()->GetCurAnisotropyQuality());
    archive->SetFastName(SETTING_QUALITY_MSAA, QualitySettingsSystem::Instance()->GetCurMSAAQuality());

    archive->SetArchive(SETTING_QUALITY_MATERIALS, new KeyedArchive);
    KeyedArchive* materialsArchive = archive->GetArchive(SETTING_QUALITY_MATERIALS);
    for (size_t i = 0; i < QualitySettingsSystem::Instance()->GetMaterialQualityGroupCount(); ++i)
    {
        FastName groupName = QualitySettingsSystem::Instance()->GetMaterialQualityGroupName(i);
        FastName groupValue = QualitySettingsSystem::Instance()->GetCurMaterialQuality(groupName);
        materialsArchive->SetFastName(groupName.c_str(), groupValue);
    }

    const ParticlesQualitySettings& particlesSettings = QualitySettingsSystem::Instance()->GetParticlesQualitySettings();
    if (particlesSettings.GetQualitiesCount() > 0)
    {
        archive->SetFastName(SETTING_QUALITY_PARTICLE, particlesSettings.GetCurrentQuality());
    }

    archive->SetArchive(SETTING_QUALITY_OPTIONS, new KeyedArchive);
    KeyedArchive* optionsArchive = archive->GetArchive(SETTING_QUALITY_OPTIONS);
    for (int32 i = 0; i < QualitySettingsSystem::Instance()->GetOptionsCount(); ++i)
    {
        FastName optionName = QualitySettingsSystem::Instance()->GetOptionName(i);
        bool optionValue = QualitySettingsSystem::Instance()->IsOptionEnabled(optionName);
        optionsArchive->SetBool(optionName.c_str(), optionValue);
    }

    appSettings.SetQualitySettings(archive);
}
}
