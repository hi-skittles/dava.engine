#include "Scene3D/Entity.h"
#include "Scene3D/Components/TextComponent.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(TextComponent)
{
    ReflectionRegistrator<TextComponent>::Begin()[M::NonExportableComponent()]
    .ConstructorByPointer()
    .Field("text", &TextComponent::GetText, &TextComponent::SetText)[M::DisplayName("Text")]
    .Field("color", &TextComponent::GetColor, &TextComponent::SetColor)[M::DisplayName("Text Color")]
    .Field("size", &TextComponent::GetSize, &TextComponent::SetSize)[M::DisplayName("Text Size"), M::Range(1.0f, 500.0f, 0.5f), M::Slider()]
    .Field("visible", &TextComponent::IsVisible, &TextComponent::SetVisible)[M::DisplayName("Show Text")]
    .End();
}

TextComponent::TextComponent()
{
}

Component* TextComponent::Clone(Entity* toEntity)
{
    TextComponent* textComponent = new TextComponent();
    textComponent->SetEntity(toEntity);

    textComponent->SetText(text);
    textComponent->SetColor(color);
    textComponent->SetSize(size);
    textComponent->SetVisible(isVisible);

    return textComponent;
}

void TextComponent::Serialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    Component::Serialize(archive, serializationContext);

    if (nullptr != archive)
    {
        archive->SetWideString("tc.text", text);
        archive->SetColor("tc.color", color);
        archive->SetFloat("tc.size", size);
        archive->SetBool("tc.visible", isVisible);
    }
}

void TextComponent::Deserialize(KeyedArchive* archive, SerializationContext* serializationContext)
{
    if (nullptr != archive)
    {
        text = archive->GetWideString("tc.text", text);
        color = archive->GetColor("tc.color", color);
        size = archive->GetFloat("tc.size", size);
        isVisible = archive->GetBool("tc.visible", isVisible);
    }

    Component::Deserialize(archive, serializationContext);
}
}
