#pragma once

#include <TArc/Controls/PropertyPanel/PropertyPanelMeta.h>

std::shared_ptr<DAVA::M::CommandProducer> CreateRemoveComponentProducer();
std::shared_ptr<DAVA::M::CommandProducer> CreateActionsEditProducer();
std::shared_ptr<DAVA::M::CommandProducer> CreateSoundsEditProducer();
std::shared_ptr<DAVA::M::CommandProducer> CreateWaveTriggerProducer();
