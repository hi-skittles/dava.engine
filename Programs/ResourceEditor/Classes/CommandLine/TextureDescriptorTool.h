#pragma once

#include <REPlatform/Global/CommandLineModule.h>

#include <TextureCompression/TextureConverter.h>

#include <Render/TextureDescriptor.h>
#include <Reflection/ReflectionRegistrator.h>

class TextureDescriptorTool : public DAVA::CommandLineModule
{
public:
    TextureDescriptorTool(const DAVA::Vector<DAVA::String>& commandLine);

private:
    bool PostInitInternal() override;
    eFrameResult OnFrameInternal() override;
    void ShowHelpInternal() override;

    void ReadCommandLine();
    bool ValidateCommandLine();

    enum eAction : DAVA::int32
    {
        ACTION_NONE = -1,

        ACTION_RESAVE_DESCRIPTORS,
        ACTION_CREATE_DESCRIPTORS,
        ACTION_SET_COMPRESSION,
        ACTION_SET_PRESET,
        ACTION_SAVE_PRESET
    };
    eAction commandAction = ACTION_NONE;

    DAVA::FilePath folderPathname;
    DAVA::FilePath filePathname;
    DAVA::FilePath presetPath;

    DAVA::FilePath filesList;
    DAVA::FilePath presetsList;

    bool forceModeEnabled = false;
    bool convertEnabled = false;
    bool generateMipMaps = false;

    DAVA::TextureConverter::eConvertQuality quality = DAVA::TextureConverter::ECQ_DEFAULT;
    DAVA::Map<DAVA::eGPUFamily, DAVA::TextureDescriptor::Compression> compressionParams;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(TextureDescriptorTool, DAVA::CommandLineModule)
    {
        DAVA::ReflectionRegistrator<TextureDescriptorTool>::Begin()[DAVA::M::CommandName("-texdescriptor")]
        .ConstructorByPointer<DAVA::Vector<DAVA::String>>()
        .End();
    }
};
