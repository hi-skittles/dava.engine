#pragma once

#include <FileSystem/FilePath.h>

#include <memory>

class SceneImporter final
{
public:
    struct ImportParams;

    SceneImporter(const DAVA::FilePath& scenePathname);
    ~SceneImporter();

    void AccumulateParams();
    void RestoreParams();

private:
    std::unique_ptr<ImportParams> params;
    DAVA::FilePath scenePathname;
};
