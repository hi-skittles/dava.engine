#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
class IClipboard;

/** Class to work with system clipboard. */
class Clipboard
{
public:
    /** Initialize work with system clipboard. */
    Clipboard();

    /** Default destructor. */
    ~Clipboard();

    /**
    Get status of clipboard helper.
    Return true if helper ready to work with clipboard.
    */
    bool IsReadyToUse() const;

    /**
    Clear system clipboard.
    Return true if successful.
    */
    bool Clear() const;

    /**
    Check that system clipboard contains Unicode text.
    Return true if successful.
    */
    bool HasText() const;

    /**
    Copy to system clipboard specified WideString `str` as Unicode string.
    Return true if successful.
    */
    bool SetText(const WideString& str);

    /** Get from system clipboard Unicode text data as WideString */
    WideString GetText() const;

private:
    std::unique_ptr<IClipboard> pImpl;
};
}
