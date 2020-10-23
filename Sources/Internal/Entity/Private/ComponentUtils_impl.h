#pragma once

namespace DAVA
{
template <typename T>
uint32 ComponentUtils::GetRuntimeId()
{
    return GetRuntimeId(Type::Instance<T>());
}

template <typename... Args>
ComponentMask ComponentUtils::MakeMask()
{
    return MakeMask((Type::Instance<Args>())...);
}

template <typename... Args>
ComponentMask ComponentUtils::MakeMask(const Args*... args)
{
    ComponentMask mask;

    for (const Type* type : { (args)... })
    {
        DVASSERT(type != nullptr);

        uint32 runtimeId = componentManager->GetRuntimeComponentId(type);

        if (runtimeId < mask.size())
        {
            DVASSERT(!mask.test(runtimeId), "Flag already set. Check arguments list.");

            mask.set(runtimeId);
        }
        else
        {
            DVASSERT(runtimeId < mask.size());
        }
    }

    DVASSERT(mask.any());

    return mask;
}
} // namespace DAVA