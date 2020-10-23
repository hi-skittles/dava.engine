#include "UI/Formula/FormulaContext.h"
#include "Reflection/ReflectedTypeDB.h"

namespace DAVA
{
using std::make_shared;
using std::shared_ptr;

FormulaContext::FormulaContext(const std::shared_ptr<FormulaContext>& parent_)
    : parent(parent_)
{
}

FormulaContext::~FormulaContext()
{
}

const std::shared_ptr<FormulaContext>& FormulaContext::GetParent() const
{
    return parent;
}

Reflection FormulaContext::FindReflection(const String& name) const
{
    Reflection res = FindReflectionLocal(name);
    if (res.IsValid())
    {
        return res;
    }

    if (GetParent() != nullptr)
    {
        return GetParent()->FindReflection(name);
    }

    return Reflection();
}

FormulaReflectionContext::FormulaReflectionContext(const Reflection& ref_, const std::shared_ptr<FormulaContext>& parent_)
    : FormulaContext(parent_)
    , reflection(ref_)
{
}

FormulaReflectionContext::~FormulaReflectionContext()
{
}

AnyFn FormulaReflectionContext::FindFunction(const String& name, const Vector<const Type*>& types) const
{
    AnyFn method = reflection.GetMethod(name);

    if (method.IsValid() && IsArgsMatchToFn(types, method))
    {
        return method;
    }

    if (GetParent() != nullptr)
    {
        return GetParent()->FindFunction(name, types);
    }

    return AnyFn();
}

Reflection FormulaReflectionContext::FindReflectionLocal(const String& name) const
{
    return reflection.GetField(name);
}

const Reflection& FormulaReflectionContext::GetReflection() const
{
    return reflection;
}

bool FormulaReflectionContext::IsArgsMatchToFn(const Vector<const Type*>& types, const AnyFn& fn) const
{
    const Vector<const Type*>& fnTypes = fn.GetInvokeParams().argsType;

    if (fnTypes.size() != types.size() + 1) // Extra param for formula context
    {
        return false;
    }

    if (fnTypes[0] != Type::Instance<std::shared_ptr<FormulaContext>>() &&
        fnTypes[0]->Deref() != Type::Instance<std::shared_ptr<FormulaContext>>())
    {
        return false;
    }

    for (std::size_t i = 0; i < types.size(); i++)
    {
        const Type* type = types[i];
        const Type* fnType = fnTypes[i + 1]->Decay();

        const Type* int32T = Type::Instance<int32>();
        const Type* float32T = Type::Instance<float32>();
        if (type != fnType && (type != int32T || fnType != float32T)) // allow conversion from int32 to float32
        {
            return false;
        }
    }

    return true;
}

FormulaFunctionContext::FormulaFunctionContext(const std::shared_ptr<FormulaContext>& parent_)
    : FormulaContext(parent_)
{
}

FormulaFunctionContext::~FormulaFunctionContext()
{
}

AnyFn FormulaFunctionContext::FindFunction(const String& name, const Vector<const Type*>& types) const
{
    auto it = functions.find(name);
    if (it != functions.end())
    {
        AnyFn res;
        for (const AnyFn& fn : it->second)
        {
            const Vector<const Type*>& fnTypes = fn.GetInvokeParams().argsType;
            if (fnTypes.size() == types.size() + 1 &&
                (fnTypes[0]->Deref() == Type::Instance<std::shared_ptr<FormulaContext>>() ||
                 fnTypes[0] == Type::Instance<std::shared_ptr<FormulaContext>>()))
            {
                static const int32 FULL_MATCH = 2;
                static const int32 NEED_CONVERTATION = 1;
                static const int32 NOT_MATCH = 0;
                int32 matchType = FULL_MATCH;

                for (std::size_t i = 0; i < types.size(); i++)
                {
                    const Type* type = types[i];
                    const Type* fnType = fn.GetInvokeParams().argsType[i + 1];

                    if (type != fnType && type->Pointer() != fnType->Pointer() && fnType != Type::Instance<Any>() && fnType->Deref() != Type::Instance<Any>())
                    {
                        if (fnType == Type::Instance<float>() && type == Type::Instance<int>())
                        {
                            matchType = NEED_CONVERTATION;
                        }
                        else
                        {
                            matchType = NOT_MATCH;
                            break;
                        }
                    }
                }

                if (matchType == FULL_MATCH)
                {
                    return fn;
                }

                if (matchType == NEED_CONVERTATION)
                {
                    res = fn;
                }
            }
        }
        return res;
    }
    return AnyFn();
}

Reflection FormulaFunctionContext::FindReflectionLocal(const String& name) const
{
    return Reflection();
}

void FormulaFunctionContext::RegisterFunction(const String& name, const AnyFn& fn)
{
    auto it = functions.find(name);
    if (it != functions.end())
    {
        it->second.push_back(fn);
    }
    else
    {
        Vector<AnyFn> vector;
        vector.push_back(fn);
        functions[name] = vector;
    }
}
}
