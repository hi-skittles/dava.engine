#include "Tests/ImGuiTest.h"
#include "Infrastructure/TestBed.h"

#include <Engine/Engine.h>
#include <Engine/EngineSettings.h>
#include <Debug/Private/ImGui.h>
#include <UI/Update/UIUpdateComponent.h>
#include <Reflection/ReflectionRegistrator.h>
#include "UI/UIControlBackground.h"

ImGuiTest::ImGuiTest(TestBed& app)
    : BaseScreen(app, "ImGuiTest")
{
    backColor = DAVA::Color(.44f, .44f, .6f, 1.f);
}

void ImGuiTest::LoadResources()
{
    BaseScreen::LoadResources();

    GetOrCreateComponent<DAVA::UIUpdateComponent>();
    DAVA::UIControlBackground* bg = GetOrCreateComponent<DAVA::UIControlBackground>();

    bg->SetDrawType(DAVA::UIControlBackground::DRAW_FILL);
    bg->SetColor(backColor);
}

void ImGuiTest::Update(DAVA::float32 timeElapsed)
{
    DVASSERT(ImGui::IsInitialized());

    if (ImGui::IsInitialized())
    {
        // 1. Show a simple window
        {
            ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiSetCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(400, 180), ImGuiSetCond_FirstUseEver);

            ImGui::Begin("Simple Window", &showAnotherWindow);

            static float f = 0.0f;
            ImGui::Text("Hello, world!");
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            if (ImGui::ColorEdit3("clear color", backColor.color))
                GetComponent<DAVA::UIControlBackground>()->SetColor(backColor);
            if (ImGui::Button("Test Window"))
                showTestWindow ^= 1;
            if (ImGui::Button("Another Window"))
                showAnotherWindow ^= 1;
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

            ImGui::End();
        }

        // 2. Show another simple window, this time using an explicit Begin/End pair
        if (showAnotherWindow)
        {
            ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
            ImGui::Begin("Another Window", &showAnotherWindow);
            ImGui::Text("Hello");
            ImGui::End();
        }

        // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
        if (showTestWindow)
        {
            ImGui::SetNextWindowPos(ImVec2(450, 20), ImGuiSetCond_FirstUseEver);
            ImGui::ShowTestWindow(&showTestWindow);
        }

        // 4. Show Engine Settings window
        {
            ShowEngineSettings();
        }
    }
}

void ImGuiTest::ShowEngineSettings()
{
    using namespace DAVA;

    ImGui::SetNextWindowPos(ImVec2(20, 240), ImGuiSetCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiSetCond_FirstUseEver);

    static Reflection settingsRefl = Reflection::Create(&app.GetEngine().GetContext()->settings);
    static Vector<Reflection::Field> settingsFields = settingsRefl.GetFields();

    ImGui::Begin("Engine Settings");

    for (const Reflection::Field& field : settingsFields)
    {
        FastName fieldname = field.key.Get<FastName>();
        const Type* fieldtype = field.ref.GetValueType()->Decay();

        if (fieldtype == Type::Instance<bool>())
        {
            bool value = field.ref.GetValue().Get<bool>();
            if (ImGui::Checkbox(fieldname.c_str(), &value))
                field.ref.SetValue(value);
        }
        else if (fieldtype == Type::Instance<int32>())
        {
            int32 value = field.ref.GetValue().Get<int32>();
            if (ImGui::InputInt(fieldname.c_str(), &value, 0))
                field.ref.SetValue(value);
        }
        else if (fieldtype == Type::Instance<float32>())
        {
            float32 value = field.ref.GetValue().Get<float32>();
            if (ImGui::InputFloat(fieldname.c_str(), &value, 0))
                field.ref.SetValue(value);
        }
        else if (fieldtype == Type::Instance<String>())
        {
            String value = field.ref.GetValue().Get<String>();

            static char buf[256] = {};
            static bool needInitBuffer = true;
            if (needInitBuffer)
            {
                DVASSERT(value.size() < sizeof(buf));
                strcpy(buf, value.c_str());
                needInitBuffer = false;
            }
            if (ImGui::InputText(fieldname.c_str(), buf, sizeof(buf)))
                field.ref.SetValue(String(buf));
        }
        else if (fieldtype == Type::Instance<EngineSettings::eSettingValue>())
        {
            using EnumSettingRange = EngineSettings::SettingRange<EngineSettings::eSettingValue>;
            const EnumSettingRange* range = field.ref.GetMeta<EnumSettingRange>();
            if (range)
            {
                int32 min = int32(range->min);
                int32 max = int32(range->max);
                int32 count = max - min + 1;
                if (count > 0)
                {
                    const char* names[EngineSettings::SETTING_VALUE_COUNT];
                    for (int32 i = 0; i < count; ++i)
                        names[i] = EngineSettings::GetSettingValueName(EngineSettings::eSettingValue(min + i)).c_str();

                    int32 current = int32(field.ref.GetValue().Get<EngineSettings::eSettingValue>()) - min;
                    if (ImGui::Combo(fieldname.c_str(), &current, names, count))
                        field.ref.SetValue(EngineSettings::eSettingValue(min + current));
                }
                else
                {
                    ImGui::Text("Invalid Range for '%s'", fieldname.c_str());
                }
            }
            else
            {
                ImGui::Text("No-Range for '%s'", fieldname.c_str());
            }
        }
        else
        {
            ImGui::Text("Unknown type for '%s'", fieldname.c_str());
        }
    }

    ImGui::End();
}
