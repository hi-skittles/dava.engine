#pragma once

#include "UI/DataBinding/Private/UIDataModel.h"

namespace DAVA
{
class FormulaContext;

class UIDataRoot : public UIDataModel
{
public:
    UIDataRoot(bool editorMode);
    ~UIDataRoot() override;

    UIComponent* GetComponent() const override;
    const std::shared_ptr<FormulaContext>& GetRootContext() const override;

    bool Process(UIDataBindingDependenciesManager* dependenciesManager) override;

private:
    // Math
    static int32 IntMin(const std::shared_ptr<FormulaContext>& context, int32 a, int32 b);
    static float32 FloatMin(const std::shared_ptr<FormulaContext>& context, float32 a, float32 b);

    static int32 IntMax(const std::shared_ptr<FormulaContext>& context, int32 a, int32 b);
    static float32 FloatMax(const std::shared_ptr<FormulaContext>& context, float32 a, float32 b);

    static int32 IntClamp(const std::shared_ptr<FormulaContext>& context, int32 val, int32 a, int32 b);
    static float32 FloatClamp(const std::shared_ptr<FormulaContext>& context, float32 val, float32 a, float32 b);

    static int32 IntAbs(const std::shared_ptr<FormulaContext>& context, int32 a);
    static float32 FloatAbs(const std::shared_ptr<FormulaContext>& context, float32 a);

    static float32 RadToDeg(const std::shared_ptr<FormulaContext>& context, float32 f);
    static float32 DegToRad(const std::shared_ptr<FormulaContext>& context, float32 f);

    // Strings
    static String IntToStr(const std::shared_ptr<FormulaContext>& context, int32 a);
    static String IntToStr1000Separated(const std::shared_ptr<FormulaContext>& context, int32 a);
    static String FloatToStr(const std::shared_ptr<FormulaContext>& context, float32 a);
    static String FloatToStrWithPrecision(const std::shared_ptr<FormulaContext>& context, float32 a, int32 precision);
    static String Float64ToStr(const std::shared_ptr<FormulaContext>& context, float64 a);
    static String Float64ToStrWithPrecision(const std::shared_ptr<FormulaContext>& context, float64 a, int32 precision);
    static String Vector2ToStr(const std::shared_ptr<FormulaContext>& context, const Vector2& value);
    static String Localize(std::shared_ptr<FormulaContext> context, const String& key);
    static String LocalizeAndSubstitudeParams(const std::shared_ptr<FormulaContext>& context, const String& str, const Any& map);
    static String SubstituteParams(const std::shared_ptr<FormulaContext>& context, const String& str, const Any& map);
    static bool StrEmpty(const std::shared_ptr<FormulaContext>& context, const String& str);

    // Conversions
    static float32 intToFloat(const std::shared_ptr<FormulaContext>& context, int32 value);
    static int32 floatToInt(const std::shared_ptr<FormulaContext>& context, float32 value);
    static float32 floor(const std::shared_ptr<FormulaContext>& context, float32 value);
};
}
