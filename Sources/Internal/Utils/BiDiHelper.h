#ifndef __DAVAENGINE_BIDIHELPER_H__
#define __DAVAENGINE_BIDIHELPER_H__

#include "Base/BaseTypes.h"

namespace DAVA
{
class BiDiWrapper;

class BiDiHelper
{
public:
    enum Direction
    {
        LTR = 0,
        RTL,
        NEUTRAL,
        MIXED
    };

    BiDiHelper();
    virtual ~BiDiHelper();

    /**
    * \brief Prepare string for BiDi transformation (shape arabic string). Need for correct splitting.
    * \param [in] logicalStr The logical string.
    * \param [out] preparedStr The prepared string.
    * \param [out] isRTL If non-null, store in isRTL true if line contains Right-to-left text.
    * \return true if it succeeds, false if it fails.
    */
    bool PrepareString(const WideString& logicalStr, WideString& preparedStr, bool* isRTL) const;

    /**
    * \brief Reorder characters in string based.
    * \param [in,out] string The string.
    * \param forceRtl (Optional) true if input text is mixed and must be processed as RTL.
    * \return true if it succeeds, false if it fails.
    */
    bool ReorderString(const WideString& preparedStr, WideString& reorderedStr, const bool forceRtl = false) const;

    bool IsBiDiSpecialCharacter(uint32 character) const;

    /** Fast detect UTF-8 string direction */
    Direction GetDirectionUTF8String(const String& utf8) const;

private:
    BiDiWrapper* wrapper;
};
}

#endif