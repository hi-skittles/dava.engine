#include "MaterialConfigCommands.h"
#include "Commands2/RECommandIDs.h"

MaterialConfigModify::MaterialConfigModify(DAVA::NMaterial* material_, int id, const DAVA::String& text)
    : RECommand(id, text)
    , material(DAVA::SafeRetain(material_))
{
    DVASSERT(material);
}

MaterialConfigModify::~MaterialConfigModify()
{
    DAVA::SafeRelease(material);
}

MaterialChangeCurrentConfig::MaterialChangeCurrentConfig(DAVA::NMaterial* material, DAVA::uint32 newCurrentConfigIndex)
    : MaterialConfigModify(material, CMDID_MATERIAL_CHANGE_CURRENT_CONFIG, "Change current material config")
    , newCurrentConfig(newCurrentConfigIndex)
    , oldCurrentConfig(material->GetCurrentConfigIndex())
{
}

void MaterialChangeCurrentConfig::Undo()
{
    material->SetCurrentConfigIndex(oldCurrentConfig);
}

void MaterialChangeCurrentConfig::Redo()
{
    material->SetCurrentConfigIndex(newCurrentConfig);
}

MaterialRemoveConfig::MaterialRemoveConfig(DAVA::NMaterial* material, DAVA::uint32 configIndex_)
    : MaterialConfigModify(material, CMDID_MATERIAL_REMOVE_CONFIG, "Remove material config")
    , config(material->GetConfig(configIndex_))
    , configIndex(configIndex_)
{
    DAVA::uint32 configCount = material->GetConfigCount();
    DAVA::uint32 newCurrConfig = material->GetCurrentConfigIndex();
    DVASSERT(configCount > 1);
    DVASSERT(configIndex < configCount);
    if ((configIndex == newCurrConfig && configIndex == configCount - 1) ||
        configIndex < newCurrConfig)
    {
        --newCurrConfig;
    }

    changeCurrentConfigCommand.reset(new MaterialChangeCurrentConfig(material, newCurrConfig));
}

void MaterialRemoveConfig::Undo()
{
    material->InsertConfig(configIndex, config);
    changeCurrentConfigCommand->Undo();
}

void MaterialRemoveConfig::Redo()
{
    material->RemoveConfig(configIndex);
    changeCurrentConfigCommand->Redo();
}

MaterialCreateConfig::MaterialCreateConfig(DAVA::NMaterial* material, const DAVA::MaterialConfig& config_)
    : MaterialConfigModify(material, CMDID_MATERIAL_CREATE_CONFIG, "Create material config")
    , config(config_)
{
    changeCurrentConfigCommand.reset(new MaterialChangeCurrentConfig(material, material->GetConfigCount()));
}

void MaterialCreateConfig::Undo()
{
    changeCurrentConfigCommand->Undo();
    DVASSERT(configIndex != -1);
    material->RemoveConfig(configIndex);
    configIndex = -1;
}

void MaterialCreateConfig::Redo()
{
    DVASSERT(configIndex == -1);
    configIndex = material->GetConfigCount();
    material->InsertConfig(configIndex, config);
    changeCurrentConfigCommand->Redo();
}
