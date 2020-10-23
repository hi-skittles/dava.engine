#pragma once

#include "TArc/DataProcessing/DataNode.h"
#include "TArc/DataProcessing/PropertiesHolder.h"

#include "FileSystem/FilePath.h"

class SpritesPackerModule;
class EditorConfig;

struct MaterialTemplateInfo
{
    DAVA::String name;
    DAVA::String path;
    DAVA::Vector<DAVA::String> qualities;
};

class ProjectManagerData : public DAVA::TArc::DataNode
{
public:
    ProjectManagerData();
    ProjectManagerData(const ProjectManagerData& other) = delete;
    ~ProjectManagerData();

    bool IsOpened() const;

    const DAVA::FilePath& GetProjectPath() const;
    DAVA::FilePath GetDataPath() const;
    DAVA::FilePath GetDataSourcePath() const;
    DAVA::FilePath GetDataSource3DPath() const;
    DAVA::FilePath GetParticlesConfigPath() const;
    DAVA::FilePath GetParticlesGfxPath() const;

    static DAVA::FilePath CreateProjectPathFromPath(const DAVA::FilePath& pathname);
    static DAVA::FilePath GetDataSourcePath(const DAVA::FilePath& pathname);
    static DAVA::FilePath GetDataPath(const DAVA::FilePath& pathname);
    const EditorConfig* GetEditorConfig() const;
    const DAVA::Vector<MaterialTemplateInfo>* GetMaterialTemplatesInfo() const;
    DAVA_DEPRECATED(const SpritesPackerModule* GetSpritesModules() const);

    static DAVA::FilePath GetDataSource3DPath(const DAVA::FilePath& projectPath);

public:
    static const DAVA::String ProjectPathProperty;

private:
    friend class ProjectManagerModule;
    friend class ProjectResources;

    std::unique_ptr<SpritesPackerModule> spritesPacker;
    std::unique_ptr<EditorConfig> editorConfig;
    DAVA::Vector<MaterialTemplateInfo> materialTemplatesInfo;

    DAVA::FilePath projectPath;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(ProjectManagerData, DAVA::TArc::DataNode)
    {
        DAVA::ReflectionRegistrator<ProjectManagerData>::Begin()
        .Field(ProjectPathProperty.c_str(), &ProjectManagerData::GetProjectPath, nullptr)
        .End();
    }
};
