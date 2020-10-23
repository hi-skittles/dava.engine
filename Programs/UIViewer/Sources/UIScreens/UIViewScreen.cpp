#include "UIScreens/UIViewScreen.h"

#include <Base/ObjectFactory.h>
#include <Base/ScopedPtr.h>
#include <CommandLine/ProgramOptions.h>
#include <Engine/Engine.h>
#include <FileSystem/FileSystem.h>
#include <FileSystem/LocalizationSystem.h>
#include <Render/2D/Systems/VirtualCoordinatesSystem.h>
#include <Render/2D/TextBlock.h>
#include <Scene3D/Systems/QualitySettingsSystem.h>
#include <Sound/SoundSystem.h>
#include <UI/DefaultUIPackageBuilder.h>
#include <UI/Flow/UIFlowStateSystem.h>
#include <UI/Layouts/UILayoutSystem.h>
#include <UI/Styles/UIStyleSheetSystem.h>
#include <UI/UIControlBackground.h>
#include <UI/UIControlSystem.h>
#include <UI/UIPackageLoader.h>
#include <UI/UIStaticText.h>
#include <UI/UIYamlLoader.h>
#include <UI/Update/UIUpdateComponent.h>
#include <Utils/UTF8Utils.h>

namespace UIViewScreenDetails
{
class PreviewPackageBuilder : public DAVA::DefaultUIPackageBuilder
{
public:
    PreviewPackageBuilder(DAVA::UIPackagesCache* packagesCache_ = nullptr)
        : DAVA::DefaultUIPackageBuilder(packagesCache_)
    {
    }

protected:
    DAVA::RefPtr<DAVA::UIControl> CreateControlByName(const DAVA::String& customClassName, const DAVA::String& className) override
    {
        using namespace DAVA;

        if (ObjectFactory::Instance()->IsTypeRegistered(customClassName))
        {
            return RefPtr<UIControl>(ObjectFactory::Instance()->New<UIControl>(customClassName));
        }

        return RefPtr<UIControl>(ObjectFactory::Instance()->New<UIControl>(className));
    }

    std::unique_ptr<DAVA::DefaultUIPackageBuilder> CreateBuilder(DAVA::UIPackagesCache* packagesCache) override
    {
        return std::make_unique<PreviewPackageBuilder>(packagesCache);
    }
};

DAVA::RefPtr<DAVA::UIControl> LoadControl(const DAVA::FilePath& yamlPath, const DAVA::String& controlName)
{
    using namespace DAVA;
    PreviewPackageBuilder packageBuilder;
    if (UIPackageLoader().LoadPackage(yamlPath, &packageBuilder))
    {
        UIControl* ctrl = packageBuilder.GetPackage()->GetControl<UIControl*>(controlName);
        return RefPtr<UIControl>(SafeRetain(ctrl));
    }

    return RefPtr<UIControl>();
}
}

UIViewScreen::UIViewScreen(DAVA::Window* window_, DAVA::ProgramOptions* options_)
    : BaseScreen()
    , window(window_)
    , options(options_)
{
    GetOrCreateComponent<DAVA::UIUpdateComponent>();

    if (options != nullptr)
    {
        projectPath = options->GetOption("-project").AsString();
    }
    else
    {
        projectPath = "~doc:/Project/";
    }
}

void UIViewScreen::LoadResources()
{
    using namespace DAVA;

    BaseScreen::LoadResources();

    FilePath::AddResourcesFolder(projectPath + "/Data/");
    FilePath::AddResourcesFolder(projectPath + "/DataSource/");

    QualitySettingsSystem::Instance()->Load("~res:/quality.yaml");

    SetupUI();

    for (RefPtr<UIControl>& c : hiddenPlaceholders)
    {
        c = new UIControl();
        UIControlBackground* bg = c->GetOrCreateComponent<UIControlBackground>();
        bg->SetDrawType(UIControlBackground::DRAW_FILL);
        bg->SetColor(Color(0.1f, 0.1f, 0.1f, 0.9f));
        AddControl(c.Get());
    }

    visibleFrameChangedToken = GetPrimaryWindow()->visibleFrameChanged.Connect([&](Window* w, Rect r) {
        Rect vr = GetScene()->vcs->ConvertInputToVirtual(r);
        Vector2 ws = GetScene()->vcs->ConvertInputToVirtual(Vector2(w->GetSize().dx, w->GetSize().dy));
        hiddenPlaceholders[PlaceholderSide::LEFT]->SetRect(Rect(0.f, 0.f, vr.x, ws.dy));
        hiddenPlaceholders[PlaceholderSide::RIGHT]->SetRect(Rect(vr.x + vr.dx, 0.f, ws.dx - (vr.x + vr.dx), ws.dy));
        hiddenPlaceholders[PlaceholderSide::TOP]->SetRect(Rect(vr.x, 0.f, vr.dx, vr.y));
        hiddenPlaceholders[PlaceholderSide::BOTTOM]->SetRect(Rect(vr.x, vr.y + vr.dy, vr.dx, ws.dy - (vr.y + vr.dy)));
    });
}

void UIViewScreen::UnloadResources()
{
    using namespace DAVA;

    if (GetPrimaryWindow())
    {
        GetPrimaryWindow()->visibleFrameChanged.Disconnect(visibleFrameChangedToken);
    }

    FilePath::RemoveResourcesFolder(projectPath + "/DataSource/");
    FilePath::RemoveResourcesFolder(projectPath + "/Data/");

    UIControlSystem* scene = GetScene() != nullptr ? GetScene() : Engine::Instance()->GetContext()->uiControlSystem;
    scene->SetFlowRoot(nullptr);
    UIFlowStateSystem* stateSystem = scene->GetSystem<UIFlowStateSystem>();
    stateSystem->DeactivateAllStates();

    BaseScreen::UnloadResources();
}

void UIViewScreen::SetupUI()
{
    using namespace DAVA;

    if (options == nullptr)
    {
        PrintError("Options were not parsed");
        return;
    }

    { //  localization
        String locale = options->GetOption("-locale").AsString();
        FilePath stringsFolder("~res:/Strings/");
        LocalizationSystem* localizationSystem = GetEngineContext()->localizationSystem;
        localizationSystem->SetDirectory(stringsFolder);
        localizationSystem->SetCurrentLocale(locale);
        localizationSystem->Init();

        { //  sound
            SoundSystem* soundSystem = GetEngineContext()->soundSystem;
            soundSystem->SetCurrentLocale(locale);
            soundSystem->InitFromQualitySettings();
        }

        { //  Fonts
            FilePath fontsConfigsDirectory = options->GetOption("-fontsDir").AsString();
            FilePath localizedFontsPath(fontsConfigsDirectory + locale + "/fonts.yaml");
            if (GetEngineContext()->fileSystem->Exists(localizedFontsPath) == false)
            {
                localizedFontsPath = fontsConfigsDirectory + "/fonts.yaml";
            }
            UIYamlLoader::LoadFonts(localizedFontsPath);
        }

        window->GetUIControlSystem()->GetStyleSheetSystem()->AddGlobalClass(FastName(locale));
    }

    { //  rtl
        bool isRtl = options->GetOption("-isRtl").AsBool();
        TextBlock::SetBiDiSupportEnabled(isRtl);
        window->GetUIControlSystem()->GetLayoutSystem()->SetRtl(isRtl);
    }

    FilePath placeHolderYaml = options->GetOption("-blankYaml").AsString();
    String placeHolderRootControl = options->GetOption("-blankRoot").AsString();
    String placeHolderPath = options->GetOption("-blankPath").AsString();

    FilePath testedYaml = options->GetOption("-testedYaml").AsString();
    String testedControlName = options->GetOption("-testedCtrl").AsString();
    String testedControlPath = options->GetOption("-testedPath").AsString();

    RefPtr<UIControl> placeHolderRoot = UIViewScreenDetails::LoadControl(placeHolderYaml, placeHolderRootControl);
    if (placeHolderRoot)
    {
        UIControl* placeholder = placeHolderRoot.Get();
        if (placeHolderPath.empty() == false)
        {
            placeholder = placeHolderRoot->FindByPath(placeHolderPath);
            if (placeholder == nullptr)
            {
                PrintError(Format("Cannot find %s in %s", placeHolderPath.c_str(), placeHolderYaml.GetStringValue().c_str()));
                return;
            }
        }

        RefPtr<UIControl> testedControl = UIViewScreenDetails::LoadControl(testedYaml, testedControlName);
        if (testedControl)
        {
            AddControl(placeHolderRoot.Get());
            if (options->GetOption("-isFlow").AsBool())
            {
                UIControlSystem* scene = GetScene() != nullptr ? GetScene() : Engine::Instance()->GetContext()->uiControlSystem;
                scene->SetFlowRoot(testedControl.Get());

                UIFlowStateSystem* stateSystem = scene->GetSystem<UIFlowStateSystem>();
                stateSystem->ActivateState(stateSystem->FindStateByPath(testedControlPath), false);
            }
            else
            {
                placeholder->AddControl(testedControl.Get());
            }
        }
        else
        {
            PrintError(Format("Cannot find %s in %s", testedControlName.c_str(), testedYaml.GetStringValue().c_str()));
        }
    }
    else
    {
        PrintError(Format("Cannot find %s in %s", placeHolderRootControl.c_str(), placeHolderYaml.GetStringValue().c_str()));
    }
}

void UIViewScreen::PrintError(const DAVA::String& errorMessage)
{
    using namespace DAVA;

    RemoveAllControls();

    ScopedPtr<UIStaticText> errorText(new UIStaticText(GetRect()));
    errorText->SetFont(font);
    errorText->SetTextColorInheritType(UIControlBackground::COLOR_IGNORE_PARENT);
    errorText->SetTextColor(Color(1.f, 0.f, 0.f, 1.f));
    errorText->SetText(UTF8Utils::EncodeToWideString(errorMessage));
    AddControl(errorText);
}
