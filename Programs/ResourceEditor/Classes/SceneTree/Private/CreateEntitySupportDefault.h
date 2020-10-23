#pragma once

#include <REPlatform/Global/SceneTree/CreateEntitySupport.h>

namespace DAVA
{
class Entity;
} // namespace DAVA

template <typename T>
class SimpleCreatorHelper : public DAVA::SimpleEntityCreator
{
public:
    SimpleCreatorHelper(DAVA::BaseEntityCreator::eMenuPointOrder order, const QIcon& icon, const QString& text)
        : SimpleEntityCreator(order, icon, text, DAVA::MakeFunction(&T::CreateEntity))
    {
    }

    DAVA_VIRTUAL_REFLECTION(SimpleCreatorHelper, DAVA::SimpleEntityCreator);
};

std::unique_ptr<DAVA::BaseEntityCreator> CreateEntityCreationTree();
