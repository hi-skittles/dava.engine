#pragma once
#include "Base/RECommand.h"
#include "Base/ScopedPtr.h"
#include "Base/RefPtr.h"

namespace DAVA
{
class KeyedArchive;
class Scene;
class NMaterial;
class SerializationContext;
class FilePath;
}

class ApplyMaterialPresetCommand : public RECommand
{
public:
    enum eMaterialPart : DAVA::uint32
    {
        NOTHING = 0,

        TEMPLATE = 1 << 0,
        GROUP = 1 << 1,
        PROPERTIES = 1 << 2,
        TEXTURES = 1 << 3,

        ALL = TEMPLATE | GROUP | PROPERTIES | TEXTURES
    };

    ApplyMaterialPresetCommand(const DAVA::FilePath& presetPath, DAVA::NMaterial* material, DAVA::Scene* scene);

    bool IsValidPreset() const;
    void Init(DAVA::uint32 materialParts);

    void Undo() override;
    void Redo() override;
    bool IsClean() const override;

    static void StoreCurrentConfigPreset(DAVA::KeyedArchive* archive, DAVA::NMaterial* material, const DAVA::SerializationContext& context);
    static void StoreAllConfigsPreset(DAVA::KeyedArchive* archive, DAVA::NMaterial* material, const DAVA::SerializationContext& context);

private:
    void LoadMaterialPreset(DAVA::KeyedArchive* archive, DAVA::uint32 parts, bool loadForUndo);
    void PrepareSerializationContext(DAVA::SerializationContext& context);

private:
    DAVA::ScopedPtr<DAVA::KeyedArchive> redoInfo;
    DAVA::ScopedPtr<DAVA::KeyedArchive> undoInfo;
    DAVA::RefPtr<DAVA::NMaterial> material;
    DAVA::Scene* scene;
    DAVA::uint32 materialParts = 0;
};
