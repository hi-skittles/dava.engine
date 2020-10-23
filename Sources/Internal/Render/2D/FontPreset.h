#pragma once

#include "Base/BaseTypes.h"
#include "Base/RefPtr.h"

namespace DAVA
{
class Font;

/** Class describes font configuration with font and its size. */
class FontPreset final
{
public:
    /** Static constant to empty preset. */
    static const FontPreset EMPTY;

    /** Default constructor. */
    FontPreset();

    /** Constructor with specified font and font's size. */
    FontPreset(const RefPtr<Font>& font, float32 size);

    /** Copy constructor. */
    FontPreset(const FontPreset& src);

    /** Destructor. */
    ~FontPreset();

    /** Return true if preset contains font. */
    bool Valid() const;

    /** Compare two presets. */
    bool operator==(const FontPreset& second) const;

    /** Return raw pointer to Font. */
    Font* GetFontPtr() const;

    /** Return refptr to Font. */
    const RefPtr<Font>& GetFont() const;

    /** Set refptr to Font. */
    void SetFont(const RefPtr<Font>& font);

    /** Return font size. */
    float32 GetSize() const;

    /** Set font size. */
    void SetSize(float32 size);

private:
    RefPtr<Font> font;
    float32 size = 14.f; // Default size
};

inline Font* FontPreset::GetFontPtr() const
{
    return font.Get();
}

inline const RefPtr<Font>& FontPreset::GetFont() const
{
    return font;
}

inline float32 FontPreset::GetSize() const
{
    return size;
}
}
