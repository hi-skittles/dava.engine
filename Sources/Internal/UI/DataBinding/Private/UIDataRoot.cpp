#include "UI/DataBinding/Private/UIDataRoot.h"

#include "UI/Formula/FormulaContext.h"
#include "UI/Formula/Private/FormulaData.h"
#include "UI/Formula/Private/FormulaFormatter.h"
#include "UI/Formula/Private/FormulaExecutor.h"

#include "FileSystem/LocalizationSystem.h"
#include "Logger/Logger.h"
#include "Math/MathHelpers.h"
#include "Reflection/Reflection.h"
#include "Reflection/ReflectedTypeDB.h"
#include "Utils/StringUtils.h"

namespace DAVA
{
UIDataRoot::UIDataRoot(bool editorMode)
    : UIDataModel(nullptr, PRIORITY_ROOT, editorMode)
{
    std::shared_ptr<FormulaFunctionContext> funcContext = std::make_shared<FormulaFunctionContext>(std::shared_ptr<FormulaContext>());
    funcContext->RegisterFunction("min", MakeFunction(&UIDataRoot::IntMin));
    funcContext->RegisterFunction("min", MakeFunction(&UIDataRoot::FloatMin));
    funcContext->RegisterFunction("max", MakeFunction(&UIDataRoot::IntMax));
    funcContext->RegisterFunction("max", MakeFunction(&UIDataRoot::FloatMax));
    funcContext->RegisterFunction("clamp", MakeFunction(&UIDataRoot::IntClamp));
    funcContext->RegisterFunction("clamp", MakeFunction(&UIDataRoot::FloatClamp));
    funcContext->RegisterFunction("abs", MakeFunction(&UIDataRoot::IntAbs));
    funcContext->RegisterFunction("abs", MakeFunction(&UIDataRoot::FloatAbs));
    funcContext->RegisterFunction("toDeg", MakeFunction(&UIDataRoot::RadToDeg));
    funcContext->RegisterFunction("toRad", MakeFunction(&UIDataRoot::DegToRad));
    funcContext->RegisterFunction("str", MakeFunction(&UIDataRoot::IntToStr));
    funcContext->RegisterFunction("str1000Separated", MakeFunction(&UIDataRoot::IntToStr1000Separated));
    funcContext->RegisterFunction("str", MakeFunction(&UIDataRoot::FloatToStr));
    funcContext->RegisterFunction("str", MakeFunction(&UIDataRoot::FloatToStrWithPrecision));
    funcContext->RegisterFunction("str", MakeFunction(&UIDataRoot::Float64ToStr));
    funcContext->RegisterFunction("str", MakeFunction(&UIDataRoot::Float64ToStrWithPrecision));
    funcContext->RegisterFunction("str", MakeFunction(&UIDataRoot::Vector2ToStr));
    funcContext->RegisterFunction("localize", MakeFunction(&UIDataRoot::Localize));
    funcContext->RegisterFunction("localize", MakeFunction(&UIDataRoot::LocalizeAndSubstitudeParams));
    funcContext->RegisterFunction("format", MakeFunction(&UIDataRoot::SubstituteParams));
    funcContext->RegisterFunction("empty", MakeFunction(&UIDataRoot::StrEmpty));
    funcContext->RegisterFunction("float", MakeFunction(&UIDataRoot::intToFloat));
    funcContext->RegisterFunction("int", MakeFunction(&UIDataRoot::floatToInt));
    funcContext->RegisterFunction("floor", MakeFunction(&UIDataRoot::floor));
    context = funcContext;
}

UIDataRoot::~UIDataRoot()
{
}

UIComponent* UIDataRoot::GetComponent() const
{
    return nullptr;
}

const std::shared_ptr<FormulaContext>& UIDataRoot::GetRootContext() const
{
    return context;
}

bool UIDataRoot::Process(UIDataBindingDependenciesManager* dependenciesManager)
{
    return UIDataModel::Process(dependenciesManager);
}

int32 UIDataRoot::IntMin(const std::shared_ptr<FormulaContext>& context, int32 a, int32 b)
{
    return Min(a, b);
}

float32 UIDataRoot::FloatMin(const std::shared_ptr<FormulaContext>& context, float32 a, float32 b)
{
    return Min(a, b);
}

int32 UIDataRoot::IntMax(const std::shared_ptr<FormulaContext>& context, int32 a, int32 b)
{
    return Max(a, b);
}

float32 UIDataRoot::FloatMax(const std::shared_ptr<FormulaContext>& context, float32 a, float32 b)
{
    return Max(a, b);
}

int32 UIDataRoot::IntClamp(const std::shared_ptr<FormulaContext>& context, int32 val, int32 a, int32 b)
{
    return Clamp(val, a, b);
}

float32 UIDataRoot::FloatClamp(const std::shared_ptr<FormulaContext>& context, float32 val, float32 a, float32 b)
{
    return Clamp(val, a, b);
}

int32 UIDataRoot::IntAbs(const std::shared_ptr<FormulaContext>& context, int32 a)
{
    return Abs(a);
}

float32 UIDataRoot::FloatAbs(const std::shared_ptr<FormulaContext>& context, float32 a)
{
    return Abs(a);
}

float32 UIDataRoot::RadToDeg(const std::shared_ptr<FormulaContext>& context, float32 f)
{
    return DAVA::RadToDeg(f);
}

float32 UIDataRoot::DegToRad(const std::shared_ptr<FormulaContext>& context, float32 f)
{
    return DAVA::DegToRad(f);
}

// Strings
String UIDataRoot::IntToStr(const std::shared_ptr<FormulaContext>& context, int32 a)
{
    return Format("%d", a);
}

String UIDataRoot::IntToStr1000Separated(const std::shared_ptr<FormulaContext>& context, int32 value)
{
    String resultStr;

    bool negative = value < 0;
    value = Abs(value);

    while (value >= 1000)
    {
        resultStr = Format(" %03d", value % 1000) + resultStr;
        value /= 1000;
    }

    value = negative ? -value : value;
    resultStr = Format("%d", value) + resultStr;

    return resultStr;
}

String UIDataRoot::FloatToStr(const std::shared_ptr<FormulaContext>& context, float32 a)
{
    return Format("%f", a);
}

String UIDataRoot::FloatToStrWithPrecision(const std::shared_ptr<FormulaContext>& context, float32 a, int32 precision)
{
    return Format("%.*f", precision, a);
}

String UIDataRoot::Float64ToStr(const std::shared_ptr<FormulaContext>& context, float64 a)
{
    return Format("%f", a);
}

String UIDataRoot::Float64ToStrWithPrecision(const std::shared_ptr<FormulaContext>& context, float64 a, int32 precision)
{
    return Format("%.*f", precision, a);
}

String UIDataRoot::Vector2ToStr(const std::shared_ptr<FormulaContext>& context, const Vector2& value)
{
    return Format("%.2f, %.2f", value.x, value.y);
}

String UIDataRoot::Localize(std::shared_ptr<FormulaContext> context, const String& key)
{
    return LocalizedUtf8String(key);
}

String UIDataRoot::LocalizeAndSubstitudeParams(const std::shared_ptr<FormulaContext>& context, const String& str, const Any& map)
{
    return SubstituteParams(context, LocalizedUtf8String(str), map);
}

String UIDataRoot::SubstituteParams(const std::shared_ptr<FormulaContext>& context, const String& str, const Any& map_)
{
    UnorderedMap<String, String> replacements;
    if (map_.CanCast<std::shared_ptr<FormulaDataMap>>())
    {
        std::shared_ptr<FormulaDataMap> map = map_.Cast<std::shared_ptr<FormulaDataMap>>();

        for (const String& key : map->GetOrderedKeys())
        {
            Any value = map->Find(key);
            if (value.CanGet<std::shared_ptr<FormulaExpression>>())
            {
                std::shared_ptr<FormulaExpression> exp = value.Get<std::shared_ptr<FormulaExpression>>();
                FormulaExecutor executor(context);
                value = executor.Calculate(exp.get());
            }
            replacements[key] = FormulaFormatter::AnyToString(value);
        }
    }
    else
    {
        Reflection ref = Reflection::Create(map_);
        Vector<Reflection::Field> fields = ref.GetFields();
        for (const Reflection::Field& f : fields)
        {
            if (f.key.CanCast<String>())
            {
                replacements[f.key.Cast<String>()] = FormulaFormatter::AnyToString(f.ref.GetValue());
            }
        }
    }

    return StringUtils::SubstituteParams(str, replacements);
}

bool UIDataRoot::StrEmpty(const std::shared_ptr<FormulaContext>& context, const String& str)
{
    return str.empty();
}

float32 UIDataRoot::intToFloat(const std::shared_ptr<FormulaContext>& context, int32 value)
{
    return static_cast<float32>(value);
}

int32 UIDataRoot::floatToInt(const std::shared_ptr<FormulaContext>& context, float32 value)
{
    return static_cast<int32>(value);
}

float32 UIDataRoot::floor(const std::shared_ptr<FormulaContext>& context, float32 value)
{
    return ::floorf(value);
}
}
