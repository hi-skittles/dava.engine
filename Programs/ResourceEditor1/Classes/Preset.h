#pragma once

namespace DAVA
{
class TextureDescriptor;
class NMaterial;
class KeyedArchive;
class FilePath;
}

namespace Preset
{
bool SaveArchive(const DAVA::KeyedArchive* presetArchive, const DAVA::FilePath& path);
DAVA::KeyedArchive* LoadArchive(const DAVA::FilePath& path);

bool ApplyTexturePreset(DAVA::TextureDescriptor* descriptor, const DAVA::KeyedArchive* preset);

bool DialogSavePresetForTexture(const DAVA::TextureDescriptor* descriptor);
bool DialogLoadPresetForTexture(DAVA::TextureDescriptor* descriptor);

bool DialogSavePresetForMaterial(DAVA::NMaterial* material);
bool DialogLoadPresetForMaterial(DAVA::NMaterial* material);
}
