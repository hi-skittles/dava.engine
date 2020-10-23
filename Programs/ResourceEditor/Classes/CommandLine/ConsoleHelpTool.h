#pragma once

#include <REPlatform/Global/CommandLineModule.h>

#include <Reflection/ReflectionRegistrator.h>

class ConsoleHelpTool : public DAVA::CommandLineModule
{
public:
    ConsoleHelpTool(const DAVA::Vector<DAVA::String>& commandLine);

private:
    eFrameResult OnFrameInternal() override;
    void ShowHelpInternal() override;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(ConsoleHelpTool, DAVA::CommandLineModule)
    {
        DAVA::ReflectionRegistrator<ConsoleHelpTool>::Begin()[DAVA::M::CommandName("-help")]
        .ConstructorByPointer<DAVA::Vector<DAVA::String>>()
        .End();
    }
};
