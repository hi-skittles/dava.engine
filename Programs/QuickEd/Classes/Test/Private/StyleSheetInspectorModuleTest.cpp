#include "TArc/Testing/TArcTestClass.h"
#include "TArc/Testing/MockDefine.h"

#include "Modules/StyleSheetInspectorModule/StyleSheetInspectorModule.h"
#include "Modules/StyleSheetInspectorModule/StyleSheetInspectorWidget.h"
#include "Modules/ProjectModule/ProjectModule.h"
#include "Modules/ProjectModule/ProjectData.h"
#include "Modules/DocumentsModule/DocumentsModule.h"
#include "Modules/DocumentsModule/DocumentData.h"

#include "Model/QuickEdPackageBuilder.h"
#include "Model/PackageHierarchy/PackageControlsNode.h"

#include "Utils/PackageListenerProxy.h"

#include "Test/Private/TestHelpers.h"
#include "Test/Private/ProjectSettingsGuard.h"
#include "Test/Private/MockDocumentsModule.h"

#include "Application/QEGlobal.h"

#include <TArc/Testing/TArcUnitTests.h>
#include <TArc/Testing/MockListener.h>
#include <TArc/Core/ContextManager.h>
#include <TArc/Utils/QtConnections.h>

#include <FileSystem/FileSystem.h>
#include <FileSystem/FilePath.h>
#include <FileSystem/File.h>

#include <UI/UIPackageLoader.h>
#include <UI/UIPackage.h>

#include <gmock/gmock.h>

namespace StyleSheetInspectorModuleTestDetails
{
class LocalMockModule;
}

DAVA_TARC_TESTCLASS(StyleSheetInspectorModuleTest)
{
    BEGIN_TESTED_MODULES();
    DECLARE_TESTED_MODULE(TestHelpers::ProjectSettingsGuard);
    DECLARE_TESTED_MODULE(TestHelpers::MockDocumentsModule);
    DECLARE_TESTED_MODULE(StyleSheetInspectorModuleTestDetails::LocalMockModule)
    DECLARE_TESTED_MODULE(ProjectModule);
    DECLARE_TESTED_MODULE(StyleSheetInspectorModule)
    END_TESTED_MODULES();

    DAVA_TEST (SelectAndCheck)
    {
        using namespace DAVA;

        using namespace ::testing;
        using namespace TestHelpers;

        EXPECT_CALL(*this, OnRowInserted())
        .Times(2)
        .WillOnce(Return())
        .WillOnce(Invoke([this]() {
            EXPECT_CALL(*this, AfterWrappersSync())
            .WillOnce(Invoke(this, &StyleSheetInspectorModuleTest::CheckSSWidget));
        }));

        StyleSheetInspectorWidget* ssWidget = FindSSWidget();
        connections.AddConnection(ssWidget->model(), &QAbstractItemModel::rowsInserted, MakeFunction(this, &StyleSheetInspectorModuleTest::OnRowInserted));

        ContextAccessor* accessor = GetAccessor();
        TEST_VERIFY(accessor->GetContextCount() == 0);

        DAVA::FilePath projectPath = TestHelpers::GetTestPath() + "StyleSheetInspectorModuleTest";

        CreateProjectFolder(projectPath);

        String projectPathStr = projectPath.GetAbsolutePathname();
        InvokeOperation(ProjectModuleTesting::CreateProjectOperation.ID, QString::fromStdString(projectPathStr));

        DataContext* globalContext = accessor->GetGlobalContext();
        ProjectData* projectData = globalContext->GetData<ProjectData>();
        TEST_VERIFY(projectData != nullptr);
        TEST_VERIFY(projectData->GetUiDirectory().absolute.IsEmpty() == false);
        TEST_VERIFY(projectData->GetProjectDirectory().IsEmpty() == false);

        FilePath documentPath(projectPath + "/DataSource/UI/test.yaml");
        InvokeOperation(QEGlobal::SelectControl.ID, documentPath, GetContextManager());
        TEST_VERIFY(accessor->GetContextCount() == 1);
    }

    void CheckSSWidget()
    {
        StyleSheetInspectorWidget* ssWidget = FindSSWidget();
        QList<QListWidgetItem*> items = ssWidget->findItems("#Control", Qt::MatchContains);
        TEST_VERIFY(items.isEmpty() == false);
        items = ssWidget->findItems("bg-drawType = DRAW_FILL", Qt::MatchContains);
        TEST_VERIFY(items.isEmpty() == false);
    }

    StyleSheetInspectorWidget* FindSSWidget() const
    {
        QList<QWidget*> foundWidgets = LookupWidget(DAVA::mainWindowKey, "Style Sheet Inspector");
        StyleSheetInspectorWidget* ssWidget = nullptr;
        for (QWidget* widget : foundWidgets)
        {
            ssWidget = dynamic_cast<StyleSheetInspectorWidget*>(widget);
            if (ssWidget != nullptr)
            {
                break;
            }
        }
        TEST_VERIFY(ssWidget != nullptr);
        return ssWidget;
    }

    DAVA::QtConnections connections;

    MOCK_METHOD0_VIRTUAL(AfterWrappersSync, void());
    MOCK_METHOD0_VIRTUAL(OnRowInserted, void());
};

namespace StyleSheetInspectorModuleTestDetails
{
class LocalMockModule : public DAVA::ClientModule
{
protected:
    void PostInit() override
    {
        using namespace DAVA;

        RegisterOperation(QEGlobal::SelectControl.ID, this, &LocalMockModule::CreateAndSelectMock);
    }

    void CreateAndSelectMock(const DAVA::FilePath& path, DAVA::ContextManager* contextManager)
    {
        using namespace DAVA;

        ContextAccessor* accessor = GetAccessor();
        TEST_VERIFY(accessor->GetContextCount() == 0);
        CreateDocument(path, contextManager);
        const DocumentData* data = accessor->GetActiveContext()->GetData<DocumentData>();
        PackageControlsNode* container = data->GetPackageNode()->GetPackageControlsNode();
        TEST_VERIFY(container->GetCount() == 1);
        ControlNode* node = container->Get(0);
        TEST_VERIFY(node != nullptr);

        SelectedNodes nodes = { node };
        DataWrapper selectionDataWrapper = accessor->CreateWrapper(DAVA::ReflectedTypeDB::Get<DocumentData>());
        selectionDataWrapper.SetFieldValue(DocumentData::selectionPropertyName, nodes);
    }

    void CreateDocument(const DAVA::FilePath& path, DAVA::ContextManager* contextManager)
    {
        using namespace DAVA;

        ContextAccessor* accessor = GetAccessor();
        FileSystem* fs = accessor->GetEngineContext()->fileSystem;
        TEST_VERIFY(fs->Exists(path) == false)
        CreateDocumentFile(path);

        QuickEdPackageBuilder builder(accessor->GetEngineContext());
        UIPackageLoader packageLoader;
        TEST_VERIFY(packageLoader.LoadPackage(path, &builder));

        RefPtr<PackageNode> package = builder.BuildPackage();
        TEST_VERIFY(package != nullptr);
        DAVA::Vector<std::unique_ptr<DAVA::TArcDataNode>> initialData;
        initialData.emplace_back(new DocumentData(package));

        DataContext::ContextID id = contextManager->CreateContext(std::move(initialData));
        contextManager->ActivateContext(id);
    }

    void CreateDocumentFile(const DAVA::FilePath& path)
    {
        using namespace DAVA;
        StringStream ss;
        ss << "Header:\n"
           << "     version: \""
           << UIPackage::CURRENT_VERSION
           << "\"\n"
           << "StyleSheets:\n"
           << "-   selector: \"#Control\"\n"
           << "    properties:\n"
           << "        bg-drawType: \"DRAW_FILL\"\n"
           << "Controls:\n"
           << "-   class: \"UIControl\"\n"
           << "    name: \"Control\"\n"
           << "    size: [32.000000, 32.000000]\n"
           << "    components:\n"
           << "        Background: {}\n";

        RefPtr<File> file(File::Create(path, File::CREATE | File::WRITE));
        TEST_VERIFY(file != nullptr);
        TEST_VERIFY(file->WriteString(ss.str(), false));
    }

    DAVA_VIRTUAL_REFLECTION(LocalMockModule, DAVA::ClientModule);
};

DAVA_VIRTUAL_REFLECTION_IMPL(LocalMockModule)
{
    DAVA::ReflectionRegistrator<LocalMockModule>::Begin()
    .ConstructorByPointer()
    .End();
}
} //namespace SSIT
