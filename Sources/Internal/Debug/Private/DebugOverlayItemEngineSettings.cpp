#include "Debug/Private/DebugOverlayItemEngineSettings.h"

#include "Engine/Engine.h"
#include "Engine/EngineSettings.h"
#include "Reflection/Reflection.h"
#include "Reflection/ReflectedTypeDB.h"
#include "Debug/DebugOverlay.h"
#include "Debug/Private/ImGui.h"

namespace DAVA
{
String DebugOverlayItemEngineSettings::GetName() const
{
    return "Engine settings";
}

void DebugOverlayItemEngineSettings::Draw()
{
    static Reflection settingsReflection = Reflection::Create(&GetEngineContext()->settings);
    static Vector<Reflection::Field> settingsFields = settingsReflection.GetFields();

    bool shown = true;
    ImGui::SetNextWindowSizeConstraints(ImVec2(400.0f, 250.0f), ImVec2(FLOAT_MAX, FLOAT_MAX));
    if (ImGui::Begin("EngineSettingsWindow", &shown, ImGuiWindowFlags_NoFocusOnAppearing))
    {
        for (const Reflection::Field& field : settingsFields)
        {
            const FastName fieldName = field.key.Get<FastName>();
            const Type* fieldType = field.ref.GetValueType()->Decay();

            if (fieldType->Is<bool>())
            {
                bool value = field.ref.GetValue().Get<bool>();
                if (ImGui::Checkbox(fieldName.c_str(), &value))
                {
                    field.ref.SetValue(value);
                }
            }
            else if (fieldType->Is<int32>())
            {
                int32 value = field.ref.GetValue().Get<int32>();
                if (ImGui::InputInt(fieldName.c_str(), &value, 0))
                {
                    field.ref.SetValue(value);
                }
            }
            else if (fieldType->Is<float32>())
            {
                float32 value = field.ref.GetValue().Get<float32>();
                if (ImGui::InputFloat(fieldName.c_str(), &value, 0))
                {
                    field.ref.SetValue(value);
                }
            }
            else if (fieldType->Is<EngineSettings::eSettingValue>())
            {
                using EnumSettingRange = Meta<EngineSettings::SettingRange<EngineSettings::eSettingValue>>;
                const EnumSettingRange* range = field.ref.GetMeta<EnumSettingRange>();
                if (range)
                {
                    int32 min = static_cast<int32>(range->min);
                    int32 max = static_cast<int32>(range->max);
                    int32 count = max - min + 1;
                    if (count > 0)
                    {
                        const char* names[EngineSettings::SETTING_VALUE_COUNT];
                        for (int32 i = 0; i < count; ++i)
                            names[i] = EngineSettings::GetSettingValueName(EngineSettings::eSettingValue(min + i)).c_str();

                        int32 current = static_cast<int32>(field.ref.GetValue().Get<EngineSettings::eSettingValue>()) - min;
                        if (ImGui::Combo(fieldName.c_str(), &current, names, count))
                        {
                            field.ref.SetValue(EngineSettings::eSettingValue(min + current));
                        }
                    }
                    else
                    {
                        ImGui::Text("Invalid range for %s", fieldName.c_str());
                    }
                }
                else
                {
                    ImGui::Text("No range defined for %s", fieldName.c_str());
                }
            }
            else
            {
                ImGui::Text("Unsupported setting type for %s", fieldName.c_str());
            }
        }
    }
    ImGui::End();

    if (!shown)
    {
        GetEngineContext()->debugOverlay->HideItem(this);
    }
}
}