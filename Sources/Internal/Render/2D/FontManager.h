#pragma once

#include "Base/BaseTypes.h"
#include "Base/RefPtr.h"
#include "Base/Singleton.h"
#include "Render/2D/FontPreset.h"

namespace DAVA
{
class Font;
class FTManager;
class FilePath;

namespace FontManagerDetails
{
struct FontConfigDescriptor;
}

class FontManager final
{
public:
    FontManager();
    ~FontManager();

    FontManager(const FontManager&) = delete;
    FontManager& operator==(const FontManager&) = delete;

    FTManager* GetFT()
    {
        return ftmanager.get();
    }

    RefPtr<Font> LoadFont(const FilePath& fontPath);

    /**
	 \brief Register font.
	 */
    void RegisterFont(Font* font);
    /**
	 \brief Unregister font.
	 */
    void UnregisterFont(Font* font);

    /**
	 \brief Register all fonts presets.
	 */
    void RegisterFontsPresets(const UnorderedMap<String, FontPreset>& presets);
    /**
	 \brief Unregister all fonts presets.
	 */
    void UnregisterFontsPresets();

    /**
	 \brief Set font preset name.
	 */
    void SetFontPreset(const FontPreset& preset, const String& name);

    /**
     \brief Get font preset name by font preset.
     */
    String GetFontPresetName(const FontPreset& preset) const;

    /**
	 \brief Get font by name.
	 */
    const FontPreset& GetFontPreset(const String& name) const;

    /**
	 \brief Get registered fonts.
	 */
    const UnorderedMap<Font*, String>& GetRegisteredFonts() const;

    /**
     \brief Get name->font map.
     */
    const UnorderedMap<String, FontPreset>& GetFontPresetMap() const;

private:
    String GetFontHashName(Font* font) const;

private:
    UnorderedMap<Font*, String> registeredFonts;
    UnorderedMap<String, FontPreset> fontPresetMap;
    UnorderedMap<String, std::unique_ptr<FontManagerDetails::FontConfigDescriptor>> fontConfigs;
    std::unique_ptr<FTManager> ftmanager;
};
};
