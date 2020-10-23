#pragma once

#include <UI/Components/UIComponent.h>

class CounterpoiseComponent final : public DAVA::UIComponent
{
    DAVA_VIRTUAL_REFLECTION(CounterpoiseComponent, DAVA::UIComponent);

    CounterpoiseComponent* Clone() const override;
    const DAVA::Type* GetType() const override;
    DAVA::int32 GetRuntimeType() const override;
};
