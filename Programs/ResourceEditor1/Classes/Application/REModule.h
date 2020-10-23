#pragma once

#include <TArc/Core/ClientModule.h>
#include <TArc/DataProcessing/DataListener.h>
#include <TArc/DataProcessing/DataWrapper.h>

#include <Render/Material/NMaterial.h>

class TextureCache;
class ResourceEditorLauncher;
class REModule : public DAVA::TArc::ClientModule
{
public:
    REModule();
    ~REModule();

protected:
    void PostInit() override;

    void ShowMaterial(DAVA::NMaterial* material);

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(REModule, DAVA::TArc::ClientModule)
    {
        DAVA::ReflectionRegistrator<REModule>::Begin()
        .ConstructorByPointer()
        .End();
    }
};