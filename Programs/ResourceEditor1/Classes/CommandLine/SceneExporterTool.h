#pragma once

#include "CommandLine/CommandLineModule.h"
#include "Utils/SceneExporter/SceneExporter.h"

#include <AssetCache/AssetCacheClient.h>

#include <Reflection/ReflectionRegistrator.h>

class SceneExporterTool : public CommandLineModule
{
public:
    SceneExporterTool(const DAVA::Vector<DAVA::String>& commandLine);

private:
    bool PostInitInternal() override;
    eFrameResult OnFrameInternal() override;
    void BeforeDestroyedInternal() override;
    void ShowHelpInternal() override;

    enum eAction : DAVA::int8
    {
        ACTION_NONE = -1,

        ACTION_EXPORT_FILE,
        ACTION_EXPORT_FOLDER,
        ACTION_EXPORT_FILELIST
    };

    SceneExporter::ExportedObjectCollection exportedObjects;
    SceneExporter::Params exportingParams;

    DAVA::AssetCacheClient::ConnectionParams connectionsParams;
    bool useAssetCache = false;

    DAVA::String filename;
    DAVA::String foldername;
    DAVA::FilePath dataSourceFolder;
    DAVA::FilePath fileListPath;

    eAction commandAction = ACTION_NONE;
    SceneExporter::eExportedObjectType commandObject = SceneExporter::OBJECT_NONE;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(SceneExporterTool, CommandLineModule)
    {
        DAVA::ReflectionRegistrator<SceneExporterTool>::Begin()
        .ConstructorByPointer<DAVA::Vector<DAVA::String>>()
        .End();
    }
};
