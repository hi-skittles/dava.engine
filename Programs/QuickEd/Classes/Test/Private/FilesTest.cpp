#include "TArc/Testing/TArcTestClass.h"

#include "Modules/ProjectModule/ProjectModule.h"
#include "Modules/ProjectModule/ProjectData.h"
#include "Modules/DocumentsModule/DocumentData.h"

#include "Model/QuickEdPackageBuilder.h"
#include "Model/YamlPackageSerializer.h"
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
#include <FileSystem/FileList.h>

#include <UI/UIPackageLoader.h>
#include <UI/UIPackage.h>

#include <Engine/Engine.h>

#include <gmock/gmock.h>

namespace FilesTestDetails
{
class LocalMockModule;
bool CopyDirectoryRecursively(const DAVA::FilePath& sourceDirectory, const DAVA::FilePath& destinationDirectory);
void CompareFiles(const DAVA::FilePath& left, const DAVA::FilePath& right);
}

DAVA_TARC_TESTCLASS(FilesTest)
{
    BEGIN_TESTED_MODULES();
    DECLARE_TESTED_MODULE(TestHelpers::ProjectSettingsGuard);
    DECLARE_TESTED_MODULE(TestHelpers::MockDocumentsModule);
    DECLARE_TESTED_MODULE(FilesTestDetails::LocalMockModule)
    DECLARE_TESTED_MODULE(ProjectModule);
    END_TESTED_MODULES();

    DAVA_TEST (TestEquality)
    {
        using namespace DAVA;

        using namespace TestHelpers;

        ContextAccessor* accessor = GetAccessor();
        TEST_VERIFY(accessor->GetContextCount() == 0);

        DAVA::FilePath projectPath = TestHelpers::GetTestPath() + "FilesTest/";

        CreateProjectFolder(projectPath);

        String projectPathStr = projectPath.GetAbsolutePathname();
        InvokeOperation(ProjectModuleTesting::CreateProjectOperation.ID, QString::fromStdString(projectPathStr));

        DataContext* globalContext = accessor->GetGlobalContext();
        ProjectData* projectData = globalContext->GetData<ProjectData>();
        TEST_VERIFY(projectData != nullptr);
        TEST_VERIFY(projectData->GetUiDirectory().absolute.IsEmpty() == false);
        TEST_VERIFY(projectData->GetProjectDirectory().IsEmpty() == false);

        FilePath testDir("~res:/QuickEd/Test/");
        TEST_VERIFY(FilesTestDetails::CopyDirectoryRecursively(testDir, projectPath));

        FilePath path("~res:/UI/TestEquality.yaml");

        InvokeOperation(QEGlobal::OpenDocumentByPath.ID, path, GetContextManager());
        TEST_VERIFY(accessor->GetContextCount() == 1);

        DataContext* activeContext = accessor->GetActiveContext();
        TEST_VERIFY(activeContext != nullptr);

        DocumentData* documentData = activeContext->GetData<DocumentData>();
        PackageNode* package = documentData->GetPackageNode();

        FilePath newPath = projectPath + "/DataSource/UI/SavedTestEquality.yaml";

        YamlPackageSerializer serializer;
        serializer.SerializePackage(package);
        TEST_VERIFY(serializer.WriteToFile(newPath));

        FileSystem* fs = accessor->GetEngineContext()->fileSystem;

        FilesTestDetails::CompareFiles(path, newPath);
    }
};

namespace FilesTestDetails
{
class LocalMockModule : public DAVA::ClientModule
{
protected:
    void PostInit() override
    {
        using namespace DAVA;

        RegisterOperation(QEGlobal::OpenDocumentByPath.ID, this, &LocalMockModule::OpenDocument);
    }

    void OpenDocument(const DAVA::FilePath& path, DAVA::ContextManager* contextManager)
    {
        using namespace DAVA;

        ContextAccessor* accessor = GetAccessor();
        const EngineContext* engineContext = accessor->GetEngineContext();
        FileSystem* fs = engineContext->fileSystem;
        TEST_VERIFY(fs->Exists(path))

        QuickEdPackageBuilder builder(engineContext);
        UIPackageLoader packageLoader;
        TEST_VERIFY(packageLoader.LoadPackage(path, &builder));

        RefPtr<PackageNode> package = builder.BuildPackage();
        TEST_VERIFY(package != nullptr);
        DAVA::Vector<std::unique_ptr<DAVA::TArcDataNode>> initialData;
        initialData.emplace_back(new DocumentData(package));

        DataContext::ContextID id = contextManager->CreateContext(std::move(initialData));
        contextManager->ActivateContext(id);
    }

    DAVA_VIRTUAL_REFLECTION(LocalMockModule, DAVA::ClientModule);
};

DAVA_VIRTUAL_REFLECTION_IMPL(LocalMockModule)
{
    DAVA::ReflectionRegistrator<LocalMockModule>::Begin()
    .ConstructorByPointer()
    .End();
}

bool CopyDirectoryRecursively(const DAVA::FilePath& sourceDirectory, const DAVA::FilePath& destinationDirectory)
{
    using namespace DAVA;

    const EngineContext* context = GetEngineContext();
    FileSystem* fs = context->fileSystem;
    DVASSERT(sourceDirectory.IsDirectoryPathname() && destinationDirectory.IsDirectoryPathname());

    if (fs->CreateDirectory(destinationDirectory, true) == FileSystem::DIRECTORY_CANT_CREATE)
    {
        return false;
    }

    bool ret = true;
    ScopedPtr<FileList> fileList(new FileList(sourceDirectory));
    int32 count = fileList->GetCount();
    for (int32 i = 0; i < count; ++i)
    {
        if (fileList->IsNavigationDirectory(i))
        {
            continue;
        }
        FilePath destinationPath = destinationDirectory + fileList->GetFilename(i);
        FilePath sourcePath = fileList->GetPathname(i);
        if (fileList->IsDirectory(i))
        {
            ret &= CopyDirectoryRecursively(sourcePath.MakeDirectoryPathname(), destinationPath.MakeDirectoryPathname());
        }
        else
        {
            ret &= fs->CopyFile(sourcePath, destinationPath, false);
        }
    }
    return ret;
}

void CompareFiles(const DAVA::FilePath& left, const DAVA::FilePath& right)
{
    using namespace DAVA;

    ScopedPtr<File> leftFile(File::Create(left, File::OPEN | File::READ));
    TEST_VERIFY(leftFile);
    ScopedPtr<File> rightFile(File::Create(left, File::OPEN | File::READ));
    TEST_VERIFY(rightFile);
    while (leftFile->IsEof() == false && rightFile->IsEof() == false)
    {
        TEST_VERIFY(leftFile->ReadLine() == rightFile->ReadLine());
    }
    TEST_VERIFY(leftFile->IsEof() && rightFile->IsEof());
}

} //namespace FilesTestDetails
