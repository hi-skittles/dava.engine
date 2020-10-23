#pragma once

#include <REPlatform/Global/CommandLineModule.h>
#include <REPlatform/Scene/Utils/SceneDumper.h>

#include <Base/BaseTypes.h>
#include <Reflection/ReflectionRegistrator.h>

class DumpTool : public DAVA::CommandLineModule
{
public:
    DumpTool(const DAVA::Vector<DAVA::String>& commandLine);

private:
    bool PostInitInternal() override;
    eFrameResult OnFrameInternal() override;
    void BeforeDestroyedInternal() override;
    void ShowHelpInternal() override;

    bool DumpSingleFile(const DAVA::FilePath& inFile, const DAVA::FilePath& outFile) const;
    bool SetResourceDir(const DAVA::FilePath& path);
    bool CreateQualityYaml(const DAVA::FilePath& path);

    enum eAction : DAVA::int32
    {
        ACTION_NONE = -1,
        ACTION_DUMP_FILES
    };
    eAction commandAction = ACTION_NONE;
    DAVA::Vector<DAVA::String> processFileList;
    DAVA::FilePath resourceDir;
    DAVA::FilePath inDir;
    DAVA::FilePath outFile;
    DAVA::FilePath outDir;

    DAVA::Vector<DAVA::eGPUFamily> compressedGPUs;
    DAVA::Vector<DAVA::String> tags;
    DAVA::SceneDumper::eMode mode = DAVA::SceneDumper::eMode::REQUIRED;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(DumpTool, DAVA::CommandLineModule)
    {
        DAVA::ReflectionRegistrator<DumpTool>::Begin()[DAVA::M::CommandName("-dump")]
        .ConstructorByPointer<DAVA::Vector<DAVA::String>>()
        .End();
    }
};
