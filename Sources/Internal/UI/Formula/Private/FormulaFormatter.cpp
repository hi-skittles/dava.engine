#include "UI/Formula/Private/FormulaFormatter.h"

#include "Utils/StringFormat.h"
#include "FileSystem/FilePath.h"

#include "Debug/DVAssert.h"

#include <cinttypes>

namespace DAVA
{
FormulaFormatter::FormulaFormatter()
{
}

FormulaFormatter::~FormulaFormatter()
{
}

String FormulaFormatter::Format(FormulaExpression* exp)
{
    exp->Accept(this);
    String result = stream.str();
    stream.clear();
    return result;
}

String FormulaFormatter::AnyToString(const Any& val)
{
    if (val.CanGet<int32>())
    {
        return DAVA::Format("%" PRId32, val.Get<int32>());
    }
    else if (val.CanGet<uint32>())
    {
        return DAVA::Format("%" PRIu32 "U", val.Get<uint32>());
    }
    else if (val.CanGet<int64>())
    {
        return DAVA::Format("%" PRId64 "L", val.Get<int64>());
    }
    else if (val.CanGet<uint64>())
    {
        return DAVA::Format("%" PRIu64 "UL", val.Get<uint64>());
    }
    else if (val.CanGet<int16>())
    {
        return DAVA::Format("%d", val.Get<int16>());
    }
    else if (val.CanGet<uint16>())
    {
        return DAVA::Format("%d", val.Get<int16>());
    }
    else if (val.CanGet<int8>())
    {
        return DAVA::Format("%d", val.Get<int8>());
    }
    else if (val.CanGet<uint8>())
    {
        return DAVA::Format("%d", val.Get<uint8>());
    }
    else if (val.CanGet<float32>())
    {
        return DAVA::Format("%f", val.Get<float32>());
    }
    else if (val.CanGet<float64>())
    {
        return DAVA::Format("%lf", val.Get<float64>());
    }
    else if (val.CanGet<String>())
    {
        return val.Get<String>();
    }
    else if (val.CanGet<FilePath>())
    {
        return val.Get<FilePath>().GetStringValue();
    }
    else if (val.CanGet<bool>())
    {
        return val.Get<bool>() ? "true" : "false";
    }
    else if (val.IsEmpty())
    {
        return "<empty>";
    }
    return "<unknown>";
}

String FormulaFormatter::AnyTypeToString(const Any& val)
{
    if (val.CanGet<int32>())
    {
        return "int32";
    }
    else if (val.CanGet<uint32>())
    {
        return "uint32";
    }
    else if (val.CanGet<int64>())
    {
        return "int64";
    }
    else if (val.CanGet<uint64>())
    {
        return "uint64";
    }
    else if (val.CanGet<int16>())
    {
        return "int16";
    }
    else if (val.CanGet<uint16>())
    {
        return "uint16";
    }
    else if (val.CanGet<int8>())
    {
        return "int8";
    }
    else if (val.CanGet<uint8>())
    {
        return "uint8";
    }
    else if (val.CanGet<float32>())
    {
        return "float32";
    }
    else if (val.CanGet<float64>())
    {
        return "float64";
    }
    else if (val.CanGet<String>())
    {
        return "String";
    }
    else if (val.CanGet<FilePath>())
    {
        return "FilePath";
    }
    else if (val.CanGet<bool>())
    {
        return "bool";
    }
    else if (val.IsEmpty())
    {
        return "<empty>";
    }
    return val.GetType()->GetName();
}

String FormulaFormatter::BinaryOpToString(FormulaBinaryOperatorExpression::Operator op)
{
    switch (op)
    {
    case FormulaBinaryOperatorExpression::OP_PLUS:
        return "+";
    case FormulaBinaryOperatorExpression::OP_MINUS:
        return "-";
    case FormulaBinaryOperatorExpression::OP_MUL:
        return "*";
    case FormulaBinaryOperatorExpression::OP_DIV:
        return "/";
    case FormulaBinaryOperatorExpression::OP_MOD:
        return "%";
    case FormulaBinaryOperatorExpression::OP_AND:
        return "and";
    case FormulaBinaryOperatorExpression::OP_OR:
        return "or";
    case FormulaBinaryOperatorExpression::OP_EQ:
        return "==";
    case FormulaBinaryOperatorExpression::OP_NOT_EQ:
        return "!=";
    case FormulaBinaryOperatorExpression::OP_LE:
        return "<=";
    case FormulaBinaryOperatorExpression::OP_LT:
        return "<";
    case FormulaBinaryOperatorExpression::OP_GE:
        return ">=";
    case FormulaBinaryOperatorExpression::OP_GT:
        return ">";

    default:
        DVASSERT("Invalid operator.");
        break;
    }
    return "";
}

void FormulaFormatter::Visit(FormulaValueExpression* exp)
{
    Any value = exp->GetValue();
    if (value.CanGet<String>())
    {
        stream << "\"" << value.Get<String>() << "\"";
    }
    else
    {
        stream << AnyToString(exp->GetValue());
    }
}

void FormulaFormatter::Visit(FormulaNegExpression* exp)
{
    stream << "-";

    bool squares = dynamic_cast<FormulaBinaryOperatorExpression*>(exp->GetExp()) != nullptr;
    if (squares)
    {
        stream << "(";
    }

    exp->GetExp()->Accept(this);

    if (squares)
    {
        stream << ")";
    }
}

void FormulaFormatter::Visit(FormulaNotExpression* exp)
{
    stream << "not ";

    bool squares = dynamic_cast<FormulaBinaryOperatorExpression*>(exp->GetExp()) != nullptr;
    if (squares)
    {
        stream << "(";
    }

    exp->GetExp()->Accept(this);

    if (squares)
    {
        stream << ")";
    }
}

void FormulaFormatter::Visit(FormulaWhenExpression* exp)
{
    stream << "when ";
    for (const auto& branch : exp->GetBranches())
    {
        branch.first->Accept(this);
        stream << " -> ";
        branch.second->Accept(this);
        stream << ", ";
    }
    exp->GetElseBranch()->Accept(this);
}

void FormulaFormatter::Visit(FormulaBinaryOperatorExpression* exp)
{
    bool lhsSq = GetExpPriority(exp->GetLhs()) > exp->GetOperatorPriority();
    bool rhsSq = GetExpPriority(exp->GetRhs()) > exp->GetOperatorPriority();

    if (lhsSq)
    {
        stream << "(";
    }
    exp->GetLhs()->Accept(this);
    if (lhsSq)
    {
        stream << ")";
    }

    stream << " " << BinaryOpToString(exp->GetOperator()) << " ";

    if (rhsSq)
    {
        stream << "(";
    }
    exp->GetRhs()->Accept(this);
    if (rhsSq)
    {
        stream << ")";
    }
}

void FormulaFormatter::Visit(FormulaFunctionExpression* exp)
{
    stream << exp->GetName();
    stream << "(";

    bool first = true;
    for (const std::shared_ptr<FormulaExpression>& param : exp->GetParms())
    {
        if (first)
        {
            first = false;
        }
        else
        {
            stream << ", ";
        }
        param->Accept(this);
    }

    stream << ")";
}

void FormulaFormatter::Visit(FormulaFieldAccessExpression* exp)
{
    if (exp->GetExp())
    {
        exp->GetExp()->Accept(this);
        stream << ".";
        stream << exp->GetFieldName();
    }
    else
    {
        stream << exp->GetFieldName();
    }
}

void FormulaFormatter::Visit(FormulaIndexExpression* exp)
{
    exp->GetExp()->Accept(this);
    stream << "[";
    exp->GetIndexExp()->Accept(this);
    stream << "]";
}

int32 FormulaFormatter::GetExpPriority(FormulaExpression* exp) const
{
    FormulaBinaryOperatorExpression* binaryExp = dynamic_cast<FormulaBinaryOperatorExpression*>(exp);
    if (binaryExp != nullptr)
    {
        return binaryExp->GetOperatorPriority();
    }
    return 0;
}
}
