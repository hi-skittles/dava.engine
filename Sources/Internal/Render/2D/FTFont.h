#ifndef __DAVAENGINE_FTFONT_H__
#define __DAVAENGINE_FTFONT_H__

#include "Base/BaseObject.h"
#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Render/2D/Font.h"
#include "Concurrency/Mutex.h"
#include "FileSystem/FilePath.h"

namespace DAVA
{
class FontManager;
class FTInternalFont;

/** 
	\ingroup fonts
	\brief Freetype-based font implementation.
 
	Class is a wrapper to freetype2 library.
 */
class FTFont : public Font
{
public:
    /**
		\brief Factory method.
		\param[in] path - path to freetype-supported file (.ttf, .otf)
		\returns constructed font
	*/
    static FTFont* Create(const FilePath& path);

    /**
		\brief Function clears cache of internal fonts
	*/
    static void ClearCache();

    virtual ~FTFont();

    /**
		\brief Clone font.
	*/
    FTFont* Clone() const override;

    /**
		\brief Tests if two fonts are the same.
	*/
    bool IsEqual(const Font* font) const override;

    /**
		\brief Get string metrics.
		\param[in] str - processed string
		\param[in, out] charSizes - if present(not NULL), will contain widths of every symbol in str
		\returns StringMetrics structure
	 */
    StringMetrics GetStringMetrics(float32 size, const WideString& str, Vector<float32>* charSizes = NULL) const override;

    /**
		\brief Get height of highest symbol in font.
		\returns height in pixels
	*/
    uint32 GetFontHeight(float32 size) const override;

    /**
		\brief Checks if symbol is present in font.
		\param[in] ch - tested symbol
		\returns true if symbol is available, false otherwise
	*/
    bool IsCharAvaliable(char16 ch) const override;

    /**
		\brief Draw string to memory buffer
		\param[in, out] buffer - destination buffer
		\param[in] bufWidth - buffer width in pixels
		\param[in] bufHeight - buffer height in pixels
		\param[in] offsetX - starting X offset
		\param[in] offsetY - starting Y offset
		\param[in] justifyWidth - TODO
		\param[in] spaceAddon - TODO
		\param[in] str - string to draw
		\param[in] contentScaleIncluded - TODO
		\returns bounding rect for string in pixels
	*/
    virtual StringMetrics DrawStringToBuffer(float32 size, void* buffer, int32 bufWidth, int32 bufHeight, int32 offsetX, int32 offsetY, int32 justifyWidth, int32 spaceAddon, const WideString& str, bool contentScaleIncluded = false);

    bool IsTextSupportsSoftwareRendering() const override;

    //We need to return font path
    const FilePath& GetFontPath() const;
    // Put font properties into YamlNode
    YamlNode* SaveToYamlNode() const override;

    void SetAscendScale(float32 ascend) override;
    float32 GetAscendScale() const override;
    void SetDescendScale(float32 ascend) override;
    float32 GetDescendScale() const override;

protected:
    // Get the raw hash string (identical for identical fonts).
    String GetRawHashString() override;

private:
    FTFont(FTInternalFont* internalFont);
    FTInternalFont* internalFont = nullptr;

    float32 ascendScale = 1.f;
    float32 descendScale = 1.f;

    FilePath fontPath;
};
};

#endif //__DAVAENGINE_FTFONT_H__
