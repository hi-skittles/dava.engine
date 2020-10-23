#pragma once

#include <REPlatform/Global/CommandLineModule.h>
#include <Reflection/ReflectionRegistrator.h>

class VersionTool : public DAVA::CommandLineModule
{
public:
    VersionTool(const DAVA::Vector<DAVA::String>& commandLine);

protected:
    eFrameResult OnFrameInternal() override;

private:
    DAVA_VIRTUAL_REFLECTION_IN_PLACE(VersionTool, DAVA::CommandLineModule)
    {
        DAVA::ReflectionRegistrator<VersionTool>::Begin()[DAVA::M::CommandName("-version")]
        .ConstructorByPointer<DAVA::Vector<DAVA::String>>()
        .End();
    }
};
