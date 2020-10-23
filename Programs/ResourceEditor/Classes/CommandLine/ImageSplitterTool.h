#pragma once

#include <REPlatform/Global/CommandLineModule.h>
#include <Reflection/ReflectionRegistrator.h>

class ImageSplitterTool : public DAVA::CommandLineModule
{
public:
    ImageSplitterTool(const DAVA::Vector<DAVA::String>& commandLine);

protected:
    bool PostInitInternal() override;
    eFrameResult OnFrameInternal() override;
    void ShowHelpInternal() override;

    DAVA::FilePath filename;
    DAVA::FilePath foldername;

    enum eAction : DAVA::int32
    {
        ACTION_NONE = -1,

        ACTION_SPLIT,
        ACTION_MERGE
    };

    eAction commandAction = ACTION_NONE;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(ImageSplitterTool, DAVA::CommandLineModule)
    {
        DAVA::ReflectionRegistrator<ImageSplitterTool>::Begin()[DAVA::M::CommandName("-imagesplitter")]
        .ConstructorByPointer<DAVA::Vector<DAVA::String>>()
        .End();
    }
};
