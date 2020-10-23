#include "Classes/Project/ProjectManagerData.h"

#include "Classes/Deprecated/EditorConfig.h"
#include "SpritesPacker/SpritesPackerModule.h"

namespace ProjectManagerDataDetails
{
const char* DATA_PATH = "Data/";
const char* DATASOURCE_PATH = "DataSource/";
const char* DATASOURCE_3D_PATH = "DataSource/3d/";
const char* PARTICLE_CONFIG_PATH = "DataSource/Configs/Particles/";
const char* PARTICLE_GFX_PATH = "DataSource/Gfx/Particles/";
}

const DAVA::String ProjectManagerData::ProjectPathProperty = DAVA::String("ProjectPath");

ProjectManagerData::ProjectManagerData()
{
}

ProjectManagerData::~ProjectManagerData() = default;

bool ProjectManagerData::IsOpened() const
{
    return (!projectPath.IsEmpty());
}

const DAVA::FilePath& ProjectManagerData::GetProjectPath() const
{
    return projectPath;
}

DAVA::FilePath ProjectManagerData::GetDataPath() const
{
    return projectPath + ProjectManagerDataDetails::DATA_PATH;
}

DAVA::FilePath ProjectManagerData::GetDataSourcePath() const
{
    return projectPath + ProjectManagerDataDetails::DATASOURCE_PATH;
}

DAVA::FilePath ProjectManagerData::GetDataSource3DPath() const
{
    return GetDataSource3DPath(projectPath);
}

DAVA::FilePath ProjectManagerData::GetDataSource3DPath(const DAVA::FilePath& projectPath)
{
    return projectPath + ProjectManagerDataDetails::DATASOURCE_3D_PATH;
}

DAVA::FilePath ProjectManagerData::GetParticlesConfigPath() const
{
    return projectPath + ProjectManagerDataDetails::PARTICLE_CONFIG_PATH;
}

DAVA::FilePath ProjectManagerData::GetParticlesGfxPath() const
{
    return projectPath + ProjectManagerDataDetails::PARTICLE_GFX_PATH;
}

DAVA::FilePath ProjectManagerData::CreateProjectPathFromPath(const DAVA::FilePath& pathname)
{
    DAVA::String fullPath = pathname.GetAbsolutePathname();
    DAVA::String::size_type pos = fullPath.find("/Data");
    if (pos != DAVA::String::npos)
    {
        return fullPath.substr(0, pos + 1);
    }

    return DAVA::FilePath();
}

DAVA::FilePath ProjectManagerData::GetDataSourcePath(const DAVA::FilePath& pathname)
{
    DAVA::String etalon = DAVA::String("/DataSource");
    DAVA::String fullPath = pathname.GetAbsolutePathname();
    DAVA::String::size_type pos = fullPath.find(etalon);
    if (pos != DAVA::String::npos)
    {
        return fullPath.substr(0, pos + etalon.size() + 1);
    }

    return DAVA::FilePath();
}

DAVA::FilePath ProjectManagerData::GetDataPath(const DAVA::FilePath& pathname)
{
    DAVA::String etalon = DAVA::String("/Data");
    DAVA::String fullPath = pathname.GetAbsolutePathname();
    DAVA::String::size_type pos = fullPath.find(etalon);
    if (pos != DAVA::String::npos)
    {
        return fullPath.substr(0, pos + etalon.size() + 1);
    }

    return DAVA::FilePath();
}

const EditorConfig* ProjectManagerData::GetEditorConfig() const
{
    return editorConfig.get();
}

const SpritesPackerModule* ProjectManagerData::GetSpritesModules() const
{
    return spritesPacker.get();
}

const DAVA::Vector<MaterialTemplateInfo>* ProjectManagerData::GetMaterialTemplatesInfo() const
{
    return &materialTemplatesInfo;
}
