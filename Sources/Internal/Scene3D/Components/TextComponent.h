#pragma once

#include "Entity/Component.h"
#include "Scene3D/SceneFile/SerializationContext.h"
#include "Reflection/Reflection.h"
#include "Math/Color.h"
#include "Base/Any.h"
#include "Base/BaseTypes.h"

namespace DAVA
{
class TextComponent : public Component
{
protected:
    ~TextComponent() override = default;

public:
    TextComponent();

    Component* Clone(Entity* toEntity) override;
    void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) override;

    const WideString& GetText() const;
    void SetText(const WideString& newText);

    const Color& GetColor() const;
    void SetColor(const Color& newColor);

    float32 GetSize() const;
    void SetSize(float32 newSize);

    bool IsVisible() const;
    void SetVisible(bool visible);

private:
    WideString text;
    Color color;

    float32 size = 1.0f;

    bool isVisible = true;

    DAVA_VIRTUAL_REFLECTION(TextComponent, Component);
};

inline const WideString& TextComponent::GetText() const
{
    return text;
}

inline void TextComponent::SetText(const WideString& newText)
{
    text = newText;
}

inline const Color& TextComponent::GetColor() const
{
    return color;
}

inline void TextComponent::SetColor(const Color& newColor)
{
    color = newColor;
}

inline float32 TextComponent::GetSize() const
{
    return size;
}

inline void TextComponent::SetSize(float32 newSize)
{
    size = newSize;
}

inline bool TextComponent::IsVisible() const
{
    return isVisible;
}

inline void TextComponent::SetVisible(bool visible)
{
    isVisible = visible;
}
}
