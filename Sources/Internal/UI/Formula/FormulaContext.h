#pragma once

#include "Reflection/Reflection.h"

namespace DAVA
{
/**
     \ingroup formula
     
     Context provides access to data and methods. Formula uses contexts to find values
     of variables or functions.
     */
class FormulaContext
{
public:
    FormulaContext(const std::shared_ptr<FormulaContext>& parent);
    virtual ~FormulaContext();

    Reflection FindReflection(const String& name) const;

    virtual AnyFn FindFunction(const String& name, const Vector<const Type*>& types) const = 0;
    virtual Reflection FindReflectionLocal(const String& name) const = 0;
    const std::shared_ptr<FormulaContext>& GetParent() const;

private:
    std::shared_ptr<FormulaContext> parent;
};

/**
     \ingroup formula
     
     Default implementation of FormulaContext which uses Reflection.
     */
class FormulaReflectionContext : public FormulaContext
{
public:
    FormulaReflectionContext(const Reflection& ref, const std::shared_ptr<FormulaContext>& parent);
    ~FormulaReflectionContext() override;

    AnyFn FindFunction(const String& name, const Vector<const Type*>& types) const override;
    Reflection FindReflectionLocal(const String& name) const override;

    const Reflection& GetReflection() const;

private:
    bool IsArgsMatchToFn(const Vector<const Type*>& types, const AnyFn& fn) const;

    Reflection reflection;
};

/**
     \ingroup formula
     
     Default implementation of FormulaContext which function overloading feature.
     */
class FormulaFunctionContext : public FormulaContext
{
public:
    FormulaFunctionContext(const std::shared_ptr<FormulaContext>& parent);
    ~FormulaFunctionContext() override;

    AnyFn FindFunction(const String& name, const Vector<const Type*>& types) const override;
    Reflection FindReflectionLocal(const String& name) const override;

    void RegisterFunction(const String& name, const AnyFn& fn);

private:
    UnorderedMap<String, Vector<AnyFn>> functions;
};
}
