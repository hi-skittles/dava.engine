#pragma once

#include <REPlatform/Global/CommandLineModule.h>

class DuplicateObjectTool : public DAVA::CommandLineModule
{
public:
    DuplicateObjectTool(const DAVA::Vector<DAVA::String>& commandLine);

private:
    bool PostInitInternal() override;
    eFrameResult OnFrameInternal() override;
    void BeforeDestroyedInternal() override;
    void ShowHelpInternal() override;

    DAVA::FilePath filePath;
    DAVA::FilePath outDirPath;
    DAVA::uint32 count = 1;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(DuplicateObjectTool, DAVA::CommandLineModule)
    {
        DAVA::ReflectionRegistrator<DuplicateObjectTool>::Begin()[DAVA::M::CommandName("-duplicate")]
        .ConstructorByPointer<DAVA::Vector<DAVA::String>>()
        .End();
    }
};

class RandomPlaceHingedEquipment : public DAVA::CommandLineModule
{
public:
    RandomPlaceHingedEquipment(const DAVA::Vector<DAVA::String>& commandLine);

private:
    bool PostInitInternal() override;
    eFrameResult OnFrameInternal() override;
    void BeforeDestroyedInternal() override;
    void ShowHelpInternal() override;

    DAVA::Vector<DAVA::FilePath> scenesPath;
    DAVA::FilePath projectRootFolder;
    DAVA::FilePath hindedEquipLibrary;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(RandomPlaceHingedEquipment, DAVA::CommandLineModule)
    {
        DAVA::ReflectionRegistrator<RandomPlaceHingedEquipment>::Begin()[DAVA::M::CommandName("-placehingedequip")]
        .ConstructorByPointer<DAVA::Vector<DAVA::String>>()
        .End();
    }
};