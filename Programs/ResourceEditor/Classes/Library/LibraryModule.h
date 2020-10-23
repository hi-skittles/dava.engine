#pragma once

#include "Classes/Library/Private/ScenePreviewDialog.h"

#include <TArc/Core/ClientModule.h>
#include <TArc/Utils/QtConnections.h>
#include <TArc/Utils/QtDelayedExecutor.h>

#include <Reflection/Reflection.h>

namespace DAVA
{
class FieldBinder;
}

class LibraryWidget;
class REFileOperationsManager;
class LibraryModule : public DAVA::ClientModule
{
public:
    ~LibraryModule() override;

protected:
    void PostInit() override;

private:
    void ShowPreview(const DAVA::FilePath& path);
    void HidePreview();

    void OnSelectedPathChanged(const DAVA::Any& selectedPathValue);

    void OnAddSceneRequested(const DAVA::FilePath& scenePathname);
    void OnEditSceneRequested(const DAVA::FilePath& scenePathname);

    void OnDAEConvertionRequested(const DAVA::FilePath& daePathname);
    void OnDAEAnimationConvertionRequested(const DAVA::FilePath& daePathname);

    void OnFBXConvertionRequested(const DAVA::FilePath& fbxPathname);
    void OnFBXAnimationConvertionRequested(const DAVA::FilePath& fbxPathname);

    void OnDoubleClicked(const DAVA::FilePath& scenePathname);
    void OnDragStarted();

    DAVA::RefPtr<ScenePreviewDialog> previewDialog;
    std::unique_ptr<DAVA::FieldBinder> fieldBinder;
    std::shared_ptr<REFileOperationsManager> fileOperationsManager;
    DAVA::QtConnections connections;
    DAVA::QtDelayedExecutor executor;

    DAVA_VIRTUAL_REFLECTION(LibraryModule, DAVA::ClientModule);
};
