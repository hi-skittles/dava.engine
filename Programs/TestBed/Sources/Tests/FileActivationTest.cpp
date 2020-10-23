#include "Tests/FileActivationTest.h"
#include "Infrastructure/TestBed.h"

#include <Base/StringStream.h>
#include <Engine/Engine.h>
#include <Logger/Logger.h>
#include <UI/Events/UIEventBindingComponent.h>
#include <UI/Render/UIDebugRenderComponent.h>
#include <UI/Update/UIUpdateComponent.h>

using namespace DAVA;

FileActivationTest::FileActivationTest(TestBed& app)
    : BaseScreen(app, "FileActivationTest")
    , engine(app.GetEngine())
{
    GetOrCreateComponent<UIUpdateComponent>();

    engine.fileActivated.Connect(this, &FileActivationTest::OnFileActivated);
}

void FileActivationTest::LoadResources()
{
    BaseScreen::LoadResources();

    DAVA::DefaultUIPackageBuilder pkgBuilder;
    DAVA::UIPackageLoader().LoadPackage("~res:/TestBed/UI/FileActivationTest.yaml", &pkgBuilder);
    DAVA::UIControl* dialog = pkgBuilder.GetPackage()->GetControl("MainFrame");
    dialog->SetSize(DAVA::Vector2(500, 500));
    AddControl(dialog);

    auto actions = dialog->GetOrCreateComponent<DAVA::UIEventBindingComponent>();
    if (actions)
    {
        actions->BindAction(DAVA::FastName("DUMP_TO_LOG"), DAVA::MakeFunction(this, &FileActivationTest::OnDumpFilenames));
    }

    textStartup = static_cast<DAVA::UIStaticText*>(dialog->FindByName("startupFiles"));
    textFiles = static_cast<DAVA::UIStaticText*>(dialog->FindByName("activationFiles"));
    textLatest = static_cast<DAVA::UIStaticText*>(dialog->FindByName("latestFiles"));

    pendingUIUpdate = true;
}

void FileActivationTest::UnloadResources()
{
    BaseScreen::UnloadResources();
}

void FileActivationTest::Update(DAVA::float32 timeElapsed)
{
    BaseScreen::Update(timeElapsed);
    if (pendingUIUpdate)
    {
        textStartup->SetUtf8Text(FormatFilenameList(app.startupActivationFilenames, 3));
        textFiles->SetUtf8Text(FormatFilenameList(activationFilenames, 5));
        textLatest->SetUtf8Text(FormatFilenameList(latestActivationFilenames, 3));

        pendingUIUpdate = false;
    }
}

void FileActivationTest::OnFileActivated(DAVA::Vector<DAVA::String> filenames)
{
    if (!latestActivationFilenames.empty())
    {
        activationFilenames.insert(std::end(activationFilenames), std::begin(latestActivationFilenames), std::end(latestActivationFilenames));
    }
    latestActivationFilenames = std::move(filenames);

    pendingUIUpdate = true;
}

void FileActivationTest::OnDumpFilenames(const DAVA::Any&)
{
    Logger::Debug("Startup activation filenames:");
    for (const String& f : app.startupActivationFilenames)
    {
        Logger::Debug("    %s", f.c_str());
    }

    Logger::Debug("Activation filenames:");
    for (const String& f : activationFilenames)
    {
        Logger::Debug("    %s", f.c_str());
    }

    Logger::Debug("Latest activation filenames:");
    for (const String& f : latestActivationFilenames)
    {
        Logger::Debug("    %s", f.c_str());
    }
}

DAVA::String FileActivationTest::FormatFilenameList(const DAVA::Vector<DAVA::String>& filenames, size_t maxRows)
{
    const size_t nfiles = filenames.size();

    StringStream ss;
    if (nfiles > 0)
    {
        size_t i = 0;
        if (nfiles > maxRows)
        {
            i = nfiles - maxRows;
        }

        if (i > 0)
        {
            ss << "..." << std::endl;
        }

        for (; i < nfiles; ++i)
        {
            ss << i + 1 << ". " << filenames[i] << std::endl;
        }
    }
    else
    {
        ss << ".. no files ..";
    }
    return ss.str();
}
