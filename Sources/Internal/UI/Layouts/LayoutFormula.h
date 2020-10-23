#pragma once

#include "Base/BaseTypes.h"
#include "UI/Formula/Formula.h"

namespace DAVA
{
class LayoutFormula final
{
public:
    LayoutFormula();
    ~LayoutFormula();

    const String& GetSource() const;
    void SetSource(const String& str);

    bool HasError() const;
    bool IsEmpty() const;

    bool HasChanges() const;
    void ResetChanges();
    void MarkChanges();

    float32 Calculate(const Reflection& ref);

    const String& GetErrorMessage() const;

private:
    String source;
    Formula formula;
    bool hasChanges = false;
    String errorMsg;
};
}
