#include "UI/Formula/Private/FormulaParser.h"

#include "Logger/Logger.h"
#include "Debug/DVAssert.h"

/**
    \ingroup formula
 
    grammar formula; // antlr EBNF format https://github.com/antlr/antlr4/blob/master/doc/index.md
 
 expression: whenExpression  | vector | map;
 
 vector: OPEN_SQUARE_BRACKET (expression SEMICOLON)* expression? CLOSE_SQUAR_BRACKET;
 map: OPEN_CURLY_BRACKET (IDENTIFIER ASSIGN expression SEMICOLON)* (IDENTIFIER ASSIGN expression)? CLOSE_CURLY_BRACKET;
 
 whenExpression : logicalOrExpression | WHEN whenEntry+ elseExpression;
 whenEntry: whenExpression ARROW whenExpression COMMA;
 elseExpression: whenExpression;
 
 logicalOrExpression: logicalAndExpression | logicalOrExpression OR logicalAndExpression;
 logicalAndExpression: equalityExpression | logicalAndExpression AND equalityExpression;
 equalityExpression: relationExpression | equalityExpression (EQ | NOT_EQ) relationExpression;
 relationExpression: additiveExpression | relationExpression (LE | LT | GE | GT) additiveExpression;
 additiveExpression: multiplicativeExpression | additiveExpression (PLUS | MINUS) multiplicativeExpression;
 multiplicativeExpression: unaryExpression | multiplicativeExpression (MUL | DIV | MOD) unaryExpression;
 unaryExpression: postfixExpression | (NOT | MINUS) unaryExpression;
 
 postfixExpression: functionExpression | accessExpression | primaryExpression;
 
 functionExpression: IDENTIFIER OPEN_BRACKET (expression (COMMA expression)*)? CLOSE_BRACKET;
 
 accessExpression:
 IDENTIFIER |
 accessExpression DOT IDENTIFIER |
 accessExpression OPEN_SQUARE_BRACKET expression CLOSE_SQUAR_BRACKET;
 
 primaryExpression: BOOL | INTEGER | STRING | FLOAT | OPEN_BRACKET expression CLOSE_BRACKET;
 
 WHEN: 'when';
 ELSE: 'else';
 BOOL: 'true' | 'false';
 STRING: '"' (~["])* '"';
 INTEGER: [0-9]+ INTEGER_SUFFIX?;
 FLOAT: [0-9]+ DOT [0-9]*;
 IDENTIFIER: ([A-Za-z] | '_') ([a-zA-Z0-9] | '_')* ;
 
 fragment INTEGER_SUFFIX: UNSIGNED_SUFFIX LONG_SUFFIX? | LONG_SUFFIX UNSIGNED_SUFFIX?;
 
 fragment UNSIGNED_SUFFIX: [uU];
 fragment LONG_SUFFIX: [lL];
 
 COMMA: ',';
 SEMICOLON: ';';
 DOT: '.';
 LT: '<';
 LE: '<=';
 GT: '>';
 GE: '>=';
 PLUS: '+';
 MINUS: '-';
 MUL: '*';
 DIV: '/';
 MOD: '%';
 ASSIGN: '=';
 EQ: '==';
 NOT_EQ: '!=';
 NOT: 'not';
 AND: 'and';
 OR: 'or';
 ARROW: '->';
 OPEN_BRACKET: '(';
 CLOSE_BRACKET: ')';
 OPEN_CURLY_BRACKET: '{';
 CLOSE_CURLY_BRACKET: '}';
 OPEN_SQUARE_BRACKET: '[';
 CLOSE_SQUAR_BRACKET: ']';
 
 WS : [ \t\r\n]+ -> skip ;

 */

namespace DAVA
{
using std::shared_ptr;
using std::make_shared;

FormulaParser::FormulaParser(const String& str)
    : tokenizer(str)
{
}

FormulaParser::~FormulaParser()
{
}

shared_ptr<FormulaExpression> FormulaParser::ParseExpression()
{
    FormulaToken token = LookToken();
    int32 lineNumber = token.GetLineNumber();
    int32 posInLine = token.GetPositionInLine();

    if (token.GetType() == FormulaToken::END)
    {
        return shared_ptr<FormulaExpression>(); // empty exp
    }
    else if (token.GetType() == FormulaToken::OPEN_CURLY_BRACKET)
    {
        NextToken();
        shared_ptr<FormulaDataMap> data = ParseMap();
        token = LookToken();
        if (token.GetType() != FormulaToken::CLOSE_CURLY_BRACKET)
        {
            DAVA_THROW(FormulaException, "'}' expected", token.GetLineNumber(), token.GetPositionInLine());
        }
        NextToken(); // close bracket
        return make_shared<FormulaValueExpression>(data, lineNumber, posInLine);
    }
    else if (token.GetType() == FormulaToken::OPEN_SQUARE_BRACKET)
    {
        NextToken();
        shared_ptr<FormulaDataVector> data = ParseVector();
        token = LookToken();
        if (token.GetType() != FormulaToken::CLOSE_SQUARE_BRACKET)
        {
            DAVA_THROW(FormulaException, "']' expected", token.GetLineNumber(), token.GetPositionInLine());
        }
        NextToken(); // close bracket
        return make_shared<FormulaValueExpression>(data, lineNumber, posInLine);
    }
    else
    {
        return ParseConditionalExpression();
    }
}

shared_ptr<FormulaDataMap> FormulaParser::ParseMap()
{
    shared_ptr<FormulaDataMap> map = make_shared<FormulaDataMap>();

    FormulaToken token = LookToken();
    bool inProgress = token.GetType() == FormulaToken::IDENTIFIER;
    while (inProgress)
    {
        token = NextToken();
        String name = GetTokenStringValue(token);

        token = NextToken();
        if (token.GetType() != FormulaToken::ASSIGN)
        {
            DAVA_THROW(FormulaException, "'=' expected", token.GetLineNumber(), token.GetPositionInLine());
        }

        token = LookToken();
        shared_ptr<FormulaExpression> exp = ParseExpression();
        if (!exp)
        {
            DAVA_THROW(FormulaException, "Map value expected", token.GetLineNumber(), token.GetPositionInLine());
        }

        if (exp->IsValue())
        {
            FormulaValueExpression* value = static_cast<FormulaValueExpression*>(exp.get());
            map->Add(name, value->GetValue());
        }
        else
        {
            map->Add(name, Any(exp));
        }

        token = LookToken();
        if (token.GetType() == FormulaToken::SEMICOLON)
        {
            NextToken(); // semicolon
            token = LookToken();
            inProgress = token.GetType() == FormulaToken::IDENTIFIER;
        }
        else
        {
            inProgress = false;
        }
    }

    return map;
}

std::shared_ptr<FormulaDataVector> FormulaParser::ParseVector()
{
    shared_ptr<FormulaDataVector> vector = make_shared<FormulaDataVector>();

    int32 index = 0;
    FormulaToken token = LookToken();
    bool inProgress = token.GetType() != FormulaToken::CLOSE_SQUARE_BRACKET;
    while (inProgress)
    {
        shared_ptr<FormulaExpression> exp = ParseExpression();
        if (!exp)
        {
            DAVA_THROW(FormulaException, "Vector value expected", token.GetLineNumber(), token.GetPositionInLine());
        }

        if (exp->IsValue())
        {
            FormulaValueExpression* value = static_cast<FormulaValueExpression*>(exp.get());
            vector->Add(value->GetValue());
        }
        else
        {
            vector->Add(Any(exp));
        }

        token = LookToken();

        if (token.GetType() == FormulaToken::SEMICOLON)
        {
            NextToken(); // semicolon
            token = LookToken();
            inProgress = token.GetType() != FormulaToken::CLOSE_SQUARE_BRACKET;
        }
        else
        {
            inProgress = false;
        }

        index++;
    }

    return vector;
}

std::shared_ptr<FormulaExpression> FormulaParser::ParseConditionalExpression()
{
    FormulaToken token = LookToken();
    if (token.GetType() == FormulaToken::WHEN)
    {
        int32 lineNumber = token.GetLineNumber();
        int32 positionNumber = token.GetPositionInLine();
        NextToken();

        Vector<std::pair<std::shared_ptr<FormulaExpression>, std::shared_ptr<FormulaExpression>>> branches;
        std::shared_ptr<FormulaExpression> elseBranch;
        while (elseBranch.get() == nullptr)
        {
            shared_ptr<FormulaExpression> exp1 = ParseConditionalExpression();

            token = LookToken();
            if (token.GetType() == FormulaToken::ARROW)
            {
                NextToken();
                shared_ptr<FormulaExpression> exp2 = ParseConditionalExpression();

                token = NextToken();
                if (token.GetType() != FormulaToken::COMMA)
                {
                    DAVA_THROW(FormulaException, "Comma expected", token.GetLineNumber(), token.GetPositionInLine());
                }
                branches.push_back(std::pair<std::shared_ptr<FormulaExpression>, std::shared_ptr<FormulaExpression>>(exp1, exp2));
            }
            else
            {
                elseBranch = exp1;
            }
        }

        if (branches.empty())
        {
            DAVA_THROW(FormulaException, "When-operator must have at least one non-else branch", lineNumber, positionNumber);
        }

        return std::make_shared<FormulaWhenExpression>(branches, elseBranch, lineNumber, positionNumber);
    }
    return ParseLogicalOr();
}

std::shared_ptr<FormulaExpression> FormulaParser::ParseLogicalOr()
{
    shared_ptr<FormulaExpression> exp1 = ParseLogicalAnd();
    FormulaToken token = LookToken();
    while (token.GetType() == FormulaToken::OR)
    {
        NextToken();
        shared_ptr<FormulaExpression> exp2 = ParseLogicalAnd();
        exp1 = make_shared<FormulaBinaryOperatorExpression>(TokenTypeToBinaryOp(token.GetType()), exp1, exp2, token.GetLineNumber(), token.GetPositionInLine());
        token = LookToken();
    }
    return exp1;
}

std::shared_ptr<FormulaExpression> FormulaParser::ParseLogicalAnd()
{
    shared_ptr<FormulaExpression> exp1 = ParseEquality();
    FormulaToken token = LookToken();
    while (token.GetType() == FormulaToken::AND)
    {
        NextToken();
        shared_ptr<FormulaExpression> exp2 = ParseEquality();
        exp1 = make_shared<FormulaBinaryOperatorExpression>(TokenTypeToBinaryOp(token.GetType()), exp1, exp2, token.GetLineNumber(), token.GetPositionInLine());
        token = LookToken();
    }
    return exp1;
}

std::shared_ptr<FormulaExpression> FormulaParser::ParseEquality()
{
    shared_ptr<FormulaExpression> exp1 = ParseRelation();
    FormulaToken token = LookToken();
    while (token.GetType() == FormulaToken::EQ ||
           token.GetType() == FormulaToken::NOT_EQ)
    {
        NextToken();
        shared_ptr<FormulaExpression> exp2 = ParseRelation();
        exp1 = make_shared<FormulaBinaryOperatorExpression>(TokenTypeToBinaryOp(token.GetType()), exp1, exp2, token.GetLineNumber(), token.GetPositionInLine());
        token = LookToken();
    }
    return exp1;
}

std::shared_ptr<FormulaExpression> FormulaParser::ParseRelation()
{
    shared_ptr<FormulaExpression> exp1 = ParseAdditive();
    FormulaToken token = LookToken();
    while (token.GetType() == FormulaToken::LE ||
           token.GetType() == FormulaToken::LT ||
           token.GetType() == FormulaToken::GE ||
           token.GetType() == FormulaToken::GT)
    {
        NextToken();
        shared_ptr<FormulaExpression> exp2 = ParseAdditive();
        exp1 = make_shared<FormulaBinaryOperatorExpression>(TokenTypeToBinaryOp(token.GetType()), exp1, exp2, token.GetLineNumber(), token.GetPositionInLine());
        token = LookToken();
    }
    return exp1;
}

std::shared_ptr<FormulaExpression> FormulaParser::ParseAdditive()
{
    shared_ptr<FormulaExpression> exp1 = ParseMultiplicative();
    FormulaToken token = LookToken();
    while (token.GetType() == FormulaToken::PLUS ||
           token.GetType() == FormulaToken::MINUS)
    {
        NextToken();
        shared_ptr<FormulaExpression> exp2 = ParseMultiplicative();
        exp1 = make_shared<FormulaBinaryOperatorExpression>(TokenTypeToBinaryOp(token.GetType()), exp1, exp2, token.GetLineNumber(), token.GetPositionInLine());
        token = LookToken();
    }
    return exp1;
}

std::shared_ptr<FormulaExpression> FormulaParser::ParseMultiplicative()
{
    shared_ptr<FormulaExpression> exp1 = ParseUnary();
    FormulaToken token = LookToken();
    while (token.GetType() == FormulaToken::MUL ||
           token.GetType() == FormulaToken::DIV ||
           token.GetType() == FormulaToken::MOD)
    {
        NextToken();
        shared_ptr<FormulaExpression> exp2 = ParseUnary();
        exp1 = make_shared<FormulaBinaryOperatorExpression>(TokenTypeToBinaryOp(token.GetType()), exp1, exp2, token.GetLineNumber(), token.GetPositionInLine());
        token = LookToken();
    }
    return exp1;
}

shared_ptr<FormulaExpression> FormulaParser::ParseUnary()
{
    FormulaToken token = LookToken();

    if (token.GetType() == FormulaToken::NOT)
    {
        NextToken();
        shared_ptr<FormulaExpression> exp = ParseUnary();
        if (!exp)
        {
            DAVA_THROW(FormulaException, "Expression expected", token.GetLineNumber(), token.GetPositionInLine());
        }

        return make_shared<FormulaNotExpression>(exp, token.GetLineNumber(), token.GetPositionInLine());
    }
    else if (token.GetType() == FormulaToken::MINUS)
    {
        NextToken();
        shared_ptr<FormulaExpression> exp = ParseUnary();
        if (!exp)
        {
            DAVA_THROW(FormulaException, "Expression expected", token.GetLineNumber(), token.GetPositionInLine());
        }

        return make_shared<FormulaNegExpression>(exp, token.GetLineNumber(), token.GetPositionInLine());
    }
    else
    {
        return ParsePostfix();
    }
}

std::shared_ptr<FormulaExpression> FormulaParser::ParsePostfix()
{
    FormulaToken token = LookToken();

    if (token.GetType() == FormulaToken::IDENTIFIER)
    {
        int32 lineNumber = token.GetLineNumber();
        int32 positionInLine = token.GetPositionInLine();

        NextToken();
        String identifier = GetTokenStringValue(token);

        token = LookToken();
        if (token.GetType() == FormulaToken::OPEN_BRACKET)
        {
            return ParseFunction(identifier, lineNumber, positionInLine);
        }
        else
        {
            return ParseAccess(identifier, lineNumber, positionInLine);
        }
    }
    else
    {
        return ParsePrimary();
    }
}

std::shared_ptr<FormulaExpression> FormulaParser::ParsePrimary()
{
    FormulaToken token = NextToken();

    switch (token.GetType())
    {
    case FormulaToken::INT32:
        return make_shared<FormulaValueExpression>(Any(token.GetInt32()), token.GetLineNumber(), token.GetPositionInLine());

    case FormulaToken::UINT32:
        return make_shared<FormulaValueExpression>(Any(token.GetUInt32()), token.GetLineNumber(), token.GetPositionInLine());

    case FormulaToken::INT64:
        return make_shared<FormulaValueExpression>(Any(token.GetInt64()), token.GetLineNumber(), token.GetPositionInLine());

    case FormulaToken::UINT64:
        return make_shared<FormulaValueExpression>(Any(token.GetUInt64()), token.GetLineNumber(), token.GetPositionInLine());

    case FormulaToken::BOOLEAN:
        return make_shared<FormulaValueExpression>(Any(token.GetBool()), token.GetLineNumber(), token.GetPositionInLine());

    case FormulaToken::FLOAT:
        return make_shared<FormulaValueExpression>(Any(token.GetFloat()), token.GetLineNumber(), token.GetPositionInLine());

    case FormulaToken::STRING:
        return make_shared<FormulaValueExpression>(Any(GetTokenStringValue(token)), token.GetLineNumber(), token.GetPositionInLine());

    case FormulaToken::OPEN_BRACKET:
    {
        shared_ptr<FormulaExpression> exp = ParseExpression();

        token = LookToken();
        if (token.GetType() != FormulaToken::CLOSE_BRACKET)
        {
            DAVA_THROW(FormulaException, "')' expected", token.GetLineNumber(), token.GetPositionInLine());
        }
        NextToken(); // close bracket
        return exp;
    }

    default:
        break;
    }

    DAVA_THROW(FormulaException, "Expected literal", token.GetLineNumber(), token.GetPositionInLine());
}

shared_ptr<FormulaExpression> FormulaParser::ParseFunction(const String& identifier, int32 lineNumber, int32 positionInLine)
{
    FormulaToken token = NextToken(); // skip open bracket

    if (token.GetType() != FormulaToken::OPEN_BRACKET)
    {
        DAVA_THROW(FormulaException, "'(' expected", token.GetLineNumber(), token.GetPositionInLine());
    }

    token = LookToken();
    Vector<shared_ptr<FormulaExpression>> params;

    if (token.GetType() == FormulaToken::CLOSE_BRACKET)
    {
        NextToken(); // skip close token
    }
    else
    {
        while (true)
        {
            params.push_back(ParseExpression());

            token = LookToken();
            if (token.GetType() == FormulaToken::COMMA)
            {
                NextToken(); // skip comma and continue
            }
            else if (token.GetType() == FormulaToken::CLOSE_BRACKET)
            {
                NextToken(); // finish function
                break;
            }
            else
            {
                DAVA_THROW(FormulaException, "expected ')'", token.GetLineNumber(), token.GetPositionInLine());
            }
        }
    }

    return make_shared<FormulaFunctionExpression>(identifier, params, lineNumber, positionInLine);
}

std::shared_ptr<FormulaExpression> FormulaParser::ParseAccess(const String& identifier, int32 lineNumber, int32 positionInLine)
{
    std::shared_ptr<FormulaExpression> exp = make_shared<FormulaFieldAccessExpression>(nullptr, identifier, lineNumber, positionInLine);

    FormulaToken token = LookToken();

    while (token.GetType() == FormulaToken::DOT || token.GetType() == FormulaToken::OPEN_SQUARE_BRACKET)
    {
        if (token.GetType() == FormulaToken::OPEN_SQUARE_BRACKET)
        {
            NextToken(); // [
            token = LookToken();
            shared_ptr<FormulaExpression> indexExp = ParseExpression();
            if (!indexExp)
            {
                DAVA_THROW(FormulaException, "Index expression expected", token.GetLineNumber(), token.GetPositionInLine());
            }
            exp = make_shared<FormulaIndexExpression>(exp, indexExp, token.GetLineNumber(), token.GetPositionInLine());

            token = NextToken();
            if (token.GetType() != FormulaToken::CLOSE_SQUARE_BRACKET)
            {
                DAVA_THROW(FormulaException, "']' expected", token.GetLineNumber(), token.GetPositionInLine());
            }
        }
        else
        {
            NextToken(); // DOT
            token = NextToken();
            if (token.GetType() == FormulaToken::IDENTIFIER)
            {
                String identifier = GetTokenStringValue(token);
                exp = make_shared<FormulaFieldAccessExpression>(exp, identifier, token.GetLineNumber(), token.GetPositionInLine());
            }
            else
            {
                DAVA_THROW(FormulaException, "Expected identifier.", token.GetLineNumber(), token.GetPositionInLine());
            }
        }

        token = LookToken();
    }
    return exp;
}

FormulaToken FormulaParser::LookToken()
{
    if (token.GetType() == FormulaToken::INVALID)
    {
        token = tokenizer.ReadToken();
    }
    return token;
}

FormulaToken FormulaParser::NextToken()
{
    if (token.GetType() != FormulaToken::INVALID)
    {
        FormulaToken result = token;
        token = FormulaToken();
        return result;
    }
    else
    {
        return tokenizer.ReadToken();
    }
}

bool FormulaParser::IsIdentifier(const FormulaToken& token, const String& identifier) const
{
    return token.GetType() == FormulaToken::IDENTIFIER && GetTokenStringValue(token) == identifier;
}

String FormulaParser::GetTokenStringValue(const FormulaToken& token) const
{
    return tokenizer.GetTokenStringValue(token);
}

FormulaBinaryOperatorExpression::Operator FormulaParser::TokenTypeToBinaryOp(FormulaToken::Type type)
{
    switch (type)
    {
    case FormulaToken::PLUS:
        return FormulaBinaryOperatorExpression::OP_PLUS;

    case FormulaToken::MINUS:
        return FormulaBinaryOperatorExpression::OP_MINUS;

    case FormulaToken::MUL:
        return FormulaBinaryOperatorExpression::OP_MUL;

    case FormulaToken::DIV:
        return FormulaBinaryOperatorExpression::OP_DIV;

    case FormulaToken::MOD:
        return FormulaBinaryOperatorExpression::OP_MOD;

    case FormulaToken::AND:
        return FormulaBinaryOperatorExpression::OP_AND;

    case FormulaToken::OR:
        return FormulaBinaryOperatorExpression::OP_OR;

    case FormulaToken::LE:
        return FormulaBinaryOperatorExpression::OP_LE;

    case FormulaToken::LT:
        return FormulaBinaryOperatorExpression::OP_LT;

    case FormulaToken::GE:
        return FormulaBinaryOperatorExpression::OP_GE;

    case FormulaToken::GT:
        return FormulaBinaryOperatorExpression::OP_GT;

    case FormulaToken::EQ:
        return FormulaBinaryOperatorExpression::OP_EQ;

    case FormulaToken::NOT_EQ:
        return FormulaBinaryOperatorExpression::OP_NOT_EQ;

    default:
        DVASSERT(false);
        return FormulaBinaryOperatorExpression::OP_PLUS;
    }
}
}
