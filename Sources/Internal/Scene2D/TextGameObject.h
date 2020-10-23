#ifndef __TEXT_GAME_OBJECT__
#define __TEXT_GAME_OBJECT__

#include "Scene2D/GameObject.h"

namespace DAVA
{
class Font;
class TextBlock;
/**
	\ingroup scene2d
	\brief represent text as game object
 */
class TextGameObject : public GameObject
{
protected:
    ~TextGameObject()
    {
    }

public: // from UIStaticText
    TextGameObject(const Rect& rect);
    TextGameObject(const Rect& rect, Font* font, const WideString& string);

    void SetText(const WideString& string, const Vector2& requestedTextRectSize = Vector2(0, 0));
    void SetFont(Font* font, bool prepareSprite = true);
    void SetMultiline(bool isMultilineEnabled, bool bySymbol = false);
    void SetFittingOption(int32 fittingType);
    void SetAlign(int32 alignment);

protected:
    void PrepareSprite();

protected:
    TextBlock* textBlock;
};
};
#endif // __TEXT_GAME_OBJECT__