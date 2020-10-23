#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

#include "Utils/BiDiHelper.h"
#include "Render/2D/TextLayout.h"

#include <float.h>

struct TestLangsData
{
    DAVA::String langId;
};

static const DAVA::Vector<TestLangsData> files = {
    TestLangsData{ "weird_characters" },
    TestLangsData{ "de" },
    TestLangsData{ "en" },
    TestLangsData{ "es" },
    TestLangsData{ "it" },
    TestLangsData{ "ru" }
};

DAVA_TESTCLASS (LocalizationTest)
{
    DAVA::FilePath srcDir;
    DAVA::FilePath cpyDir;

    LocalizationTest()
    {
        using namespace DAVA;
        srcDir = "~res:/TestData/LocalizationTest/";
        cpyDir = FileSystem::Instance()->GetCurrentDocumentsDirectory() + "LocalizationTest/";

        FileSystem::Instance()->DeleteDirectory(cpyDir);
        FileSystem::eCreateDirectoryResult createDone = FileSystem::Instance()->CreateDirectory(cpyDir, true);
        TEST_VERIFY(createDone != FileSystem::DIRECTORY_CANT_CREATE);
    }

    ~LocalizationTest()
    {
        using namespace DAVA;
        FileSystem::Instance()->DeleteDirectory(cpyDir, true);

        // Return back default locale. Let other tests work nice.
        LocalizationSystem* localizationSystem = LocalizationSystem::Instance();
        localizationSystem->InitWithDirectory("~res:/Strings/");
    }

    DAVA_TEST (LocaleTest)
    {
        using namespace DAVA;

        LocalizationSystem* localizationSystem = LocalizationSystem::Instance();

        String locale = localizationSystem->GetDeviceLocale();
        Logger::FrameworkDebug("Current locale is %s", locale.c_str());

        localizationSystem->Cleanup();

        for (size_t i = 0; i < files.size(); ++i)
        {
            const String& currentLangId = files[i].langId;
            FilePath srcFile = srcDir + (currentLangId + ".yaml");
            FilePath cpyFile = cpyDir + (currentLangId + ".yaml");

            bool copyDone = FileSystem::Instance()->CopyFile(srcFile, cpyFile, true);
            TEST_VERIFY(copyDone);

            localizationSystem->InitWithDirectory(cpyDir);

            bool setLocaleDone = localizationSystem->SetCurrentLocale(currentLangId);
            TEST_VERIFY(setLocaleDone);

            bool saveDone = localizationSystem->SaveLocalizedStrings();
            TEST_VERIFY(saveDone);

            localizationSystem->Cleanup();
            TEST_VERIFY_WITH_MESSAGE(FileSystem::Instance()->CompareTextFiles(srcFile, cpyFile), Format("Localization test: %s", files[i].langId.c_str()));
        }
    }

// TODO: linux
#if !defined(__DAVAENGINE_LINUX__)
    DAVA_TEST (BiDiTest)
    {
        using namespace DAVA;
        BiDiHelper helper;
        TextLayout layout(true);

        Font* font = FTFont::Create("~res:/Fonts/korinna.ttf");

        FilePath filePath("~res:/TestData/LocalizationTest/bidi_test.yaml");
        RefPtr<YamlParser> parser = YamlParser::Create(filePath);
        SCOPE_EXIT
        {
            SafeRelease(font);
        };

        TEST_VERIFY_WITH_MESSAGE(parser != nullptr, Format("Failed to open yaml file: %s", filePath.GetAbsolutePathname().c_str()));
        if (parser == nullptr)
            return;

        YamlNode* rootNode = parser->GetRootNode();
        TEST_VERIFY_WITH_MESSAGE(rootNode != nullptr, Format("Empty YAML file: %s", filePath.GetAbsolutePathname().c_str()));
        if (rootNode == nullptr)
            return;

        uint32 cnt = rootNode->GetCount();
        for (uint32 k = 0; k < cnt; ++k)
        {
            const YamlNode* node = rootNode->Get(k);
            const YamlNode* inputNode = node->Get("input");
            const YamlNode* visualNode = node->Get("visual");

            TEST_VERIFY_WITH_MESSAGE(inputNode != nullptr, Format("YamlNode %d: input node is empty", k));
            TEST_VERIFY_WITH_MESSAGE(visualNode != nullptr, Format("YamlNode %d: visual node is empty", k));
            if (inputNode == nullptr || visualNode == nullptr)
                break;

            WideString input = inputNode->AsWString();
            WideString visual = visualNode->AsWString();
            WideString visual_work;

            layout.Reset(input, *font, 14.f);
            while (!layout.IsEndOfText())
            {
                layout.NextByWords(FLT_MAX);
                visual_work += layout.GetVisualLine(!layout.IsEndOfText());
                if (!layout.IsEndOfText())
                {
                    // Paste linebreak for comparing splinted strings and string from config
                    visual_work += L"\n";
                }
            }
            TEST_VERIFY_WITH_MESSAGE(visual == visual_work, Format("YamlNode index: %d", k));
        }
    }
#endif
};
