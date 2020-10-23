#pragma once

#include <UI/Components/UIComponent.h>

class MovableInEditorComponent final : public DAVA::UIComponent
{
    DAVA_VIRTUAL_REFLECTION(MovableInEditorComponent, DAVA::UIComponent);

    MovableInEditorComponent* Clone() const override;
    const DAVA::Type* GetType() const override;
    DAVA::int32 GetRuntimeType() const override;
};
