#include "sl_GeneratorGLES.h"
#include "sl_Parser.h"
#include "sl_Tree.h"

#include "Logger/Logger.h"

#include <stdarg.h>

namespace sl
{
static const HLSLType kBoolType(HLSLBaseType_Bool);

static const char* _builtInSemantics[] =
{
  "SV_POSITION", "gl_Position",
  "DEPTH", "gl_FragDepth",
};

// These are reserved words in GLSL that aren't reserved in HLSL.
const char* GLESGenerator::ReservedWord[] =
{
  "output",
  "input",
  "mod",
  "mix",
  "fract"
};

const char* GLESGenerator::GetTypeName(const HLSLType& type)
{
    switch (type.baseType)
    {
    case HLSLBaseType_Void:
        return "void";
    case HLSLBaseType_Float:
        return "float";
    case HLSLBaseType_Float2:
        return "vec2";
    case HLSLBaseType_Float3:
        return "vec3";
    case HLSLBaseType_Float4:
        return "vec4";
    case HLSLBaseType_Float3x3:
        return "mat3";
    case HLSLBaseType_Float4x4:
        return "mat4";
    case HLSLBaseType_Half:
        return "float";
    case HLSLBaseType_Half2:
        return "vec2";
    case HLSLBaseType_Half3:
        return "vec3";
    case HLSLBaseType_Half4:
        return "vec4";
    case HLSLBaseType_Half3x3:
        return "mat3";
    case HLSLBaseType_Half4x4:
        return "mat4";
    case HLSLBaseType_Bool:
        return "bool";
    case HLSLBaseType_Int:
        return "int";
    case HLSLBaseType_Int2:
        return "ivec2";
    case HLSLBaseType_Int3:
        return "ivec3";
    case HLSLBaseType_Int4:
        return "ivec4";
    case HLSLBaseType_Uint:
        return "uint";
    case HLSLBaseType_Uint2:
        return "uvec2";
    case HLSLBaseType_Uint3:
        return "uvec3";
    case HLSLBaseType_Uint4:
        return "uvec4";
    case HLSLBaseType_Texture:
        return "texture";
    case HLSLBaseType_Sampler:
        return "sampler";
    case HLSLBaseType_Sampler2D:
        return "sampler2D";
    case HLSLBaseType_Sampler2DShadow:
        return "sampler2DShadow";
    case HLSLBaseType_Sampler3D:
        return "sampler3D";
    case HLSLBaseType_SamplerCube:
        return "samplerCube";
    case HLSLBaseType_UserDefined:
        return type.typeName;
    default:
        break; // to shut up goddamn warning
    }
    DVASSERT(0);
    return "?";
}

static bool GetCanImplicitCast(const HLSLType& srcType, const HLSLType& dstType)
{
    return srcType.baseType == dstType.baseType;
}

static const char* GetBuiltInSemantic(const char* semantic)
{
    int numBuiltInSemantics = (sizeof(_builtInSemantics) / sizeof(_builtInSemantics[0])) / 2;
    for (int i = 0; i < numBuiltInSemantics; ++i)
    {
        if (String_EqualNoCase(semantic, _builtInSemantics[i * 2 + 0]))
        {
            return _builtInSemantics[i * 2 + 1];
        }
    }
    return NULL;
}

int GLESGenerator::GetFunctionArguments(HLSLFunctionCall* functionCall, HLSLExpression* expression[], int maxArguments)
{
    HLSLExpression* argument = functionCall->argument;
    int numArguments = 0;
    while (argument != NULL)
    {
        if (numArguments < maxArguments)
        {
            expression[numArguments] = argument;
        }
        argument = argument->nextExpression;
        ++numArguments;
    }
    return numArguments;
}

GLESGenerator::GLESGenerator(Allocator* allocator)
{
    tree = NULL;
    entryName = NULL;
    target = TARGET_VERTEX;
    inAttribPrefix = NULL;
    outAttribPrefix = NULL;
    hasError = false;
    matrixRowFunction[0] = 0;
    clipFunction[0] = 0;
    tex2DlodFunction[0] = 0;
    tex2DbiasFunction[0] = 0;
    tex3DlodFunction[0] = 0;
    texCUBEbiasFunction[0] = 0;
    scalarSwizzle2Function[0] = 0;
    scalarSwizzle3Function[0] = 0;
    scalarSwizzle4Function[0] = 0;
    sinCosFunction[0] = 0;
    outputPosition = false;
    mrtUsed = false;
}

bool GLESGenerator::Generate(const HLSLTree* tree_, Target target_, const char* entryName_, std::string* code)
{
    tree = tree_;
    entryName = entryName_;
    target = target_;
    mrtUsed = false;

    writer.Reset(code);

    HLSLStruct* fragment_out = tree->FindGlobalStruct("fragment_out");
    if (fragment_out)
    {
        for (HLSLStructField* f = fragment_out->field; f; f = f->nextField)
        {
            if (f->semantic && stricmp(f->semantic, "SV_TARGET1") == 0)
            {
                mrtUsed = true;
                break;
            }
        }
    }

    bool usesClip = tree->GetContainsString("clip");
    bool usesTex2Dlod = tree->GetContainsString("tex2Dlod");
    bool usesTex2Dbias = tree->GetContainsString("tex2Dbias");
    bool usesTex3Dlod = tree->GetContainsString("tex3Dlod");
    bool usestexCUBEbias = tree->GetContainsString("texCUBEbias");
    bool usesSinCos = tree->GetContainsString("sincos");


    #if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_ANDROID__)
    writer.WriteLine(0, "precision highp float;");
    #else
    writer.WriteLine(0, "#define highp ");
    writer.WriteLine(0, "#define mediump ");
    writer.WriteLine(0, "#define lowp ");
    #endif
    writer.WriteLine(0, "#define FP_A8(t) (t).a");

    #if defined(__DAVAENGINE_MACOS__)
    writer.WriteLine(0, "#define lerp(a,b,t) ( ( (b) - (a) ) * (t) + (a) )");
    #endif

    ChooseUniqueName("matrix_row", matrixRowFunction, sizeof(matrixRowFunction));
    ChooseUniqueName("clip", clipFunction, sizeof(clipFunction));
    ChooseUniqueName("tex2Dlod", tex2DlodFunction, sizeof(tex2DlodFunction));
    ChooseUniqueName("tex2Dbias", tex2DbiasFunction, sizeof(tex2DbiasFunction));
    ChooseUniqueName("tex3Dlod", tex3DlodFunction, sizeof(tex3DlodFunction));
    ChooseUniqueName("texCUBEbias", texCUBEbiasFunction, sizeof(texCUBEbiasFunction));

    for (int i = 0; i < NumReservedWords; ++i)
    {
        ChooseUniqueName(ReservedWord[i], reservedWord[i], sizeof(reservedWord[i]));
    }

    ChooseUniqueName("m_scalar_swizzle2", scalarSwizzle2Function, sizeof(scalarSwizzle2Function));
    ChooseUniqueName("m_scalar_swizzle3", scalarSwizzle3Function, sizeof(scalarSwizzle3Function));
    ChooseUniqueName("m_scalar_swizzle4", scalarSwizzle4Function, sizeof(scalarSwizzle4Function));

    ChooseUniqueName("sincos", sinCosFunction, sizeof(sinCosFunction));

    if (target == TARGET_VERTEX)
    {
        inAttribPrefix = "";
        outAttribPrefix = "frag_";
    }
    else
    {
        inAttribPrefix = "frag_";
        outAttribPrefix = "rast_";
    }

    HLSLRoot* root = tree->GetRoot();
    HLSLStatement* statement = root->statement;

    // Find the entry point function.
    HLSLFunction* entryFunction = FindFunction(root, entryName);
    if (entryFunction == NULL)
    {
        Error("Entry point '%s' doesn't exist", entryName);
        return false;
    }

    //    DVASSERT(entryFunction->numArguments==1);
    HLSLArgument* arg = entryFunction->argument;
    HLSLStruct* in_struct = tree->FindGlobalStruct(arg->type.typeName);

    if (tree->FindGlobalStruct("vertex_in"))
    {
        for (HLSLStructField* field = in_struct->field; field; field = field->nextField)
        {
            const char* tname = GetTypeName(field->type);

            if (field->semantic && (stricmp(field->semantic, "COLOR") == 0 || stricmp(field->semantic, "COLOR0") == 0 || stricmp(field->semantic, "COLOR1") == 0))
                tname = "vec4";

            if (field->semantic && (stricmp(field->semantic, "POSITION") == 0))
                tname = "vec4";

            writer.WriteLine(0, "attribute %s attr_%s;", tname, field->name);
        }
    }
    else
    {
        for (HLSLStructField* field = in_struct->field; field; field = field->nextField)
        {
            const char* prefix = "";
            const char* tname = GetTypeName(field->type);

            if (field->attribute)
            {
                if (stricmp(field->attribute->attrText, "lowp") == 0)
                    prefix = "lowp";
            }
            else
            {
                if (field->type.baseType == HLSLBaseType_Half || field->type.baseType == HLSLBaseType_Half2 || field->type.baseType == HLSLBaseType_Half3 || field->type.baseType == HLSLBaseType_Half4 || field->type.baseType == HLSLBaseType_Half3x3 || field->type.baseType == HLSLBaseType_Half4x4)
                    prefix = "mediump";
            }

            writer.WriteLine(0, "varying %s %s var_%s;", prefix, tname, field->name);
        }
    }

    if (target == TARGET_VERTEX)
    {
        for (HLSLStatement* s = entryFunction->statement; s; s = s->nextStatement)
        {
            if (s->nodeType == HLSLNodeType_ReturnStatement)
            {
                HLSLReturnStatement* ret = static_cast<HLSLReturnStatement*>(s);
                HLSLStruct* out_struct = tree->FindGlobalStruct(ret->expression->expressionType.typeName);

                for (HLSLStructField* field = out_struct->field; field; field = field->nextField)
                {
                    if (stricmp(field->semantic, "SV_POSITION") != 0
                        && stricmp(field->semantic, "SV_TARGET") != 0
                        )
                    {
                        const char* prefix = "";
                        const char* tname = GetTypeName(field->type);

                        if (field->attribute)
                        {
                            if (stricmp(field->attribute->attrText, "lowp") == 0)
                                prefix = "lowp";
                        }
                        else
                        {
                            if (field->type.baseType == HLSLBaseType_Half || field->type.baseType == HLSLBaseType_Half2 || field->type.baseType == HLSLBaseType_Half3 || field->type.baseType == HLSLBaseType_Half4)
                                prefix = "mediump";
                        }

                        writer.WriteLine(0, "varying %s %s var_%s;", prefix, tname, field->name);
                    }
                }
            }
        }
    }

    OutputStatements(0, statement);

    writer.WriteLine(0, entryFunction->fileName, entryFunction->line, "void");
    writer.WriteLine(0, "main()");
    writer.WriteLine(0, "{");
    writer.EndLine();
    OutputStatements(1, entryFunction->statement);
    writer.WriteLine(0, "}");

    tree = NULL;

    // The GLSL compilers don't check for this, so generate our own error message.
    /*
    if (target == Target_VertexShader && !m_outputPosition)
    {
        Error("Vertex shader must output a position");
    }
*/
    return !hasError;
}

const char* GLESGenerator::GetResult() const
{
    return writer.GetResult();
}

void GLESGenerator::OutputExpressionList(HLSLExpression* expression, HLSLArgument* argument)
{
    int numExpressions = 0;
    while (expression != NULL)
    {
        if (numExpressions > 0)
        {
            writer.Write(", ");
        }

        HLSLType* expectedType = NULL;
        if (argument != NULL)
        {
            expectedType = &argument->type;
            argument = argument->nextArgument;
        }

        OutputExpression(expression, expectedType);
        expression = expression->nextExpression;
        ++numExpressions;
    }
}

void GLESGenerator::OutputExpression(HLSLExpression* expression, const HLSLType* dstType)
{
    bool cast = dstType != NULL && !GetCanImplicitCast(expression->expressionType, *dstType);
    if (expression->nodeType == HLSLNodeType_CastingExpression)
    {
        // No need to include a cast if the expression is already doing it.
        cast = false;
    }

    if (cast)
    {
        OutputDeclaration(*dstType, "");
        writer.Write("(");
    }

    if (expression->nodeType == HLSLNodeType_IdentifierExpression)
    {
        HLSLIdentifierExpression* identifierExpression = static_cast<HLSLIdentifierExpression*>(expression);

        const char* name = identifierExpression->name;

        if (identifierExpression->expressionType.baseType == HLSLBaseType_Sampler2D
            || identifierExpression->expressionType.baseType == HLSLBaseType_SamplerCube
            || identifierExpression->expressionType.baseType == HLSLBaseType_Sampler2DShadow
            )
        {
            HLSLDeclaration* declaration = tree->FindGlobalDeclaration(name);
            DVASSERT(declaration);
            int reg = -1;

            if (declaration->registerName != NULL)
            {
                sscanf(declaration->registerName, "s%d", &reg);
            }
            DVASSERT(reg != -1);

            const char* tname = (tree->FindGlobalStruct("vertex_in")) ? "Vertex" : "Fragment";

            writer.Write("%sTexture%d", tname, reg);
        }
        else
        {
            OutputIdentifier(name);
        }
    }
    else if (expression->nodeType == HLSLNodeType_ConstructorExpression)
    {
        HLSLConstructorExpression* constructorExpression = static_cast<HLSLConstructorExpression*>(expression);
        writer.Write("%s(", GetTypeName(constructorExpression->type));
        OutputExpressionList(constructorExpression->argument);
        writer.Write(")");
    }
    else if (expression->nodeType == HLSLNodeType_CastingExpression)
    {
        HLSLCastingExpression* castingExpression = static_cast<HLSLCastingExpression*>(expression);
        OutputDeclaration(castingExpression->type, "");
        writer.Write("(");
        OutputExpression(castingExpression->expression);
        writer.Write(")");
    }
    else if (expression->nodeType == HLSLNodeType_LiteralExpression)
    {
        HLSLLiteralExpression* literalExpression = static_cast<HLSLLiteralExpression*>(expression);
        switch (literalExpression->type)
        {
        case HLSLBaseType_Half:
        case HLSLBaseType_Float:
        {
            // Don't use printf directly so that we don't use the system locale.
            char buffer[64];
            String_FormatFloat(buffer, sizeof(buffer), literalExpression->fValue);
            writer.Write("%s", buffer);
        }
        break;
        case HLSLBaseType_Int:
        case HLSLBaseType_Uint:
            writer.Write("%d", literalExpression->iValue);
            break;
        case HLSLBaseType_Bool:
            writer.Write("%s", literalExpression->bValue ? "true" : "false");
            break;
        default:
            DVASSERT(0);
        }
    }
    else if (expression->nodeType == HLSLNodeType_UnaryExpression)
    {
        HLSLUnaryExpression* unaryExpression = static_cast<HLSLUnaryExpression*>(expression);
        const char* op = "?";
        bool pre = true;
        const HLSLType* dstType = NULL;
        switch (unaryExpression->unaryOp)
        {
        case HLSLUnaryOp_Negative:
            op = "-";
            break;
        case HLSLUnaryOp_Positive:
            op = "+";
            break;
        case HLSLUnaryOp_Not:
            op = "!";
            dstType = &unaryExpression->expressionType;
            break;
        case HLSLUnaryOp_PreIncrement:
            op = "++";
            break;
        case HLSLUnaryOp_PreDecrement:
            op = "--";
            break;
        case HLSLUnaryOp_PostIncrement:
            op = "++";
            pre = false;
            break;
        case HLSLUnaryOp_PostDecrement:
            op = "--";
            pre = false;
            break;
        default:
            break; // to shut up goddamn warning
        }
        writer.Write("(");
        if (pre)
        {
            writer.Write("%s", op);
            OutputExpression(unaryExpression->expression, dstType);
        }
        else
        {
            OutputExpression(unaryExpression->expression, dstType);
            writer.Write("%s", op);
        }
        writer.Write(")");
    }
    else if (expression->nodeType == HLSLNodeType_BinaryExpression)
    {
        HLSLBinaryExpression* binaryExpression = static_cast<HLSLBinaryExpression*>(expression);
        const char* op = "?";
        const HLSLType* dstType1 = NULL;
        const HLSLType* dstType2 = NULL;
        switch (binaryExpression->binaryOp)
        {
        case HLSLBinaryOp_Add:
            op = " + ";
            dstType1 = dstType2 = &binaryExpression->expressionType;
            break;
        case HLSLBinaryOp_Sub:
            op = " - ";
            dstType1 = dstType2 = &binaryExpression->expressionType;
            break;
        case HLSLBinaryOp_Mul:
            op = " * ";
            break;
        case HLSLBinaryOp_Div:
            op = " / ";
            break;
        case HLSLBinaryOp_Less:
            op = " < ";
            break;
        case HLSLBinaryOp_Greater:
            op = " > ";
            break;
        case HLSLBinaryOp_LessEqual:
            op = " <= ";
            break;
        case HLSLBinaryOp_GreaterEqual:
            op = " >= ";
            break;
        case HLSLBinaryOp_Equal:
            op = " == ";
            break;
        case HLSLBinaryOp_NotEqual:
            op = " != ";
            break;
        case HLSLBinaryOp_Assign:
            op = " = ";
            dstType2 = &binaryExpression->expressionType;
            break;
        case HLSLBinaryOp_AddAssign:
            op = " += ";
            dstType2 = &binaryExpression->expressionType;
            break;
        case HLSLBinaryOp_SubAssign:
            op = " -= ";
            dstType2 = &binaryExpression->expressionType;
            break;
        case HLSLBinaryOp_MulAssign:
            op = " *= ";
            dstType2 = &binaryExpression->expressionType;
            break;
        case HLSLBinaryOp_DivAssign:
            op = " /= ";
            dstType2 = &binaryExpression->expressionType;
            break;
        case HLSLBinaryOp_And:
            op = " && ";
            dstType1 = dstType2 = &binaryExpression->expressionType;
            break;
        case HLSLBinaryOp_Or:
            op = " || ";
            dstType1 = dstType2 = &binaryExpression->expressionType;
            break;
        default:
            DVASSERT(0);
        }
        writer.Write("(");
        OutputExpression(binaryExpression->expression1, dstType1);
        writer.Write("%s", op);
        OutputExpression(binaryExpression->expression2, dstType2);
        writer.Write(")");
    }
    else if (expression->nodeType == HLSLNodeType_ConditionalExpression)
    {
        HLSLConditionalExpression* conditionalExpression = static_cast<HLSLConditionalExpression*>(expression);
        writer.Write("((");
        OutputExpression(conditionalExpression->condition, &kBoolType);
        writer.Write(")?(");
        OutputExpression(conditionalExpression->trueExpression);
        writer.Write("):(");
        OutputExpression(conditionalExpression->falseExpression);
        writer.Write("))");
    }
    else if (expression->nodeType == HLSLNodeType_MemberAccess)
    {
        HLSLMemberAccess* memberAccess = static_cast<HLSLMemberAccess*>(expression);
        bool do_out_exspr = true;

        if (memberAccess->object->nodeType == HLSLNodeType_IdentifierExpression)
        {
            HLSLIdentifierExpression* identifier = static_cast<HLSLIdentifierExpression*>(memberAccess->object);
            StructUsage usage = struct_Generic;

            if (identifier->expressionType.baseType == HLSLBaseType_UserDefined)
            {
                HLSLStruct* s = tree->FindGlobalStruct(identifier->expressionType.typeName);

                if (s)
                    usage = s->usage;
            }

            if (usage == struct_VertexOut || usage == struct_FragmentOut)
            {
                HLSLStruct* out_struct = tree->FindGlobalStruct(memberAccess->object->expressionType.typeName);

                for (HLSLStructField* f = out_struct->field; f; f = f->nextField)
                {
                    if (strcmp(f->name, memberAccess->field) == 0)
                    {
                        if (f->semantic && stricmp(f->semantic, "SV_POSITION") == 0)
                        {
                            writer.Write("gl_Position");
                        }
                        else if (f->semantic && (stricmp(f->semantic, "SV_TARGET") == 0 || stricmp(f->semantic, "SV_TARGET0") == 0 || stricmp(f->semantic, "SV_TARGET1") == 0 || stricmp(f->semantic, "SV_TARGET2") == 0 || stricmp(f->semantic, "SV_TARGET3") == 0))
                        {
                            if (mrtUsed)
                            {
                                if (stricmp(f->semantic, "SV_TARGET0") == 0)
                                    writer.Write("gl_FragData[0]");
                                else if (stricmp(f->semantic, "SV_TARGET1") == 0)
                                    writer.Write("gl_FragData[1]");
                                else if (stricmp(f->semantic, "SV_TARGET2") == 0)
                                    writer.Write("gl_FragData[2]");
                                else if (stricmp(f->semantic, "SV_TARGET3") == 0)
                                    writer.Write("gl_FragData[3]");
                            }
                            else
                            {
                                writer.Write("gl_FragColor");
                            }
                        }
                        else
                        {
                            writer.Write("var_%s", f->name);
                        }

                        do_out_exspr = false;
                        break;
                    }
                }
            }
            else if (usage == struct_VertexIn)
            {
                HLSLStruct* in_struct = tree->FindGlobalStruct(memberAccess->object->expressionType.typeName);

                for (HLSLStructField* f = in_struct->field; f; f = f->nextField)
                {
                    if (strcmp(f->name, memberAccess->field) == 0)
                    {
                        writer.Write("attr_%s", f->name);

                        do_out_exspr = false;
                        break;
                    }
                }
            }
            else if (usage == struct_FragmentIn)
            {
                HLSLStruct* in_struct = tree->FindGlobalStruct(memberAccess->object->expressionType.typeName);

                for (HLSLStructField* f = in_struct->field; f; f = f->nextField)
                {
                    if (strcmp(f->name, memberAccess->field) == 0)
                    {
                        writer.Write("var_%s", f->name);

                        do_out_exspr = false;
                        break;
                    }
                }
            }
        }

        if (do_out_exspr)
        {
            if (memberAccess->object->expressionType.baseType == HLSLBaseType_Half ||
                memberAccess->object->expressionType.baseType == HLSLBaseType_Float ||
                memberAccess->object->expressionType.baseType == HLSLBaseType_Int ||
                memberAccess->object->expressionType.baseType == HLSLBaseType_Uint)
            {
                // Handle swizzling on scalar values.
                int swizzleLength = int(strlen(memberAccess->field));
                if (swizzleLength == 2)
                {
                    writer.Write("%s", scalarSwizzle2Function);
                }
                else if (swizzleLength == 3)
                {
                    writer.Write("%s", scalarSwizzle3Function);
                }
                else if (swizzleLength == 4)
                {
                    writer.Write("%s", scalarSwizzle4Function);
                }
                writer.Write("(");
                OutputExpression(memberAccess->object);
                writer.Write(")");
            }
            else
            {
                writer.Write("(");
                OutputExpression(memberAccess->object);
                writer.Write(")");
                writer.Write(".%s", memberAccess->field);
            }
        }
    }
    else if (expression->nodeType == HLSLNodeType_ArrayAccess)
    {
        HLSLArrayAccess* arrayAccess = static_cast<HLSLArrayAccess*>(expression);

        OutputExpression(arrayAccess->array);
        writer.Write("[");
        OutputExpression(arrayAccess->index);
        writer.Write("]");
    }
    else if (expression->nodeType == HLSLNodeType_FunctionCall)
    {
        HLSLFunctionCall* functionCall = static_cast<HLSLFunctionCall*>(expression);

        // Handle intrinsic functions that are different between HLSL and GLSL.
        bool handled = false;
        const char* functionName = functionCall->function->name;

        if (String_Equal(functionName, "mul"))
        {
            HLSLExpression* argument[2];
            if (GetFunctionArguments(functionCall, argument, 2) != 2)
            {
                Error("mul expects 2 arguments");
                return;
            }
            writer.Write("((");
            OutputExpression(argument[1], &functionCall->function->argument->nextArgument->type);
            writer.Write(") * (");
            OutputExpression(argument[0], &functionCall->function->argument->type);
            writer.Write("))");
            handled = true;
        }
        else if (String_Equal(functionName, "saturate"))
        {
            HLSLExpression* argument[1];
            if (GetFunctionArguments(functionCall, argument, 1) != 1)
            {
                Error("saturate expects 1 argument");
                return;
            }
            writer.Write("clamp(");
            OutputExpression(argument[0]);
            writer.Write(", 0.0, 1.0)");
            handled = true;
        }

        if (!handled)
        {
            OutputIdentifier(functionName);
            writer.Write("(");
            OutputExpressionList(functionCall->argument, functionCall->function->argument);
            writer.Write(")");
        }
    }
    else
    {
        writer.Write("<unknown expression>");
    }

    if (cast)
    {
        /*
        const BaseTypeDescription& srcTypeDesc = _baseTypeDescriptions[expression->expressionType.baseType];
        const BaseTypeDescription& dstTypeDesc = _baseTypeDescriptions[dstType->baseType];

        if (dstTypeDesc.numDimensions == 1 && dstTypeDesc.numComponents > 1)
        {
            // Casting to a vector - pad with 0s
            for (int i = srcTypeDesc.numComponents; i < dstTypeDesc.numComponents; ++i)
            {
                m_writer.Write(", 0");
            }
        }
*/

        writer.Write(")");
    }
}

void GLESGenerator::OutputIdentifier(const char* name)
{
    // Remap intrinstic functions.
    if (String_Equal(name, "tex2D"))
    {
        name = "texture2D";
    }
    else if (String_Equal(name, "tex2Dcmp"))
    {
        name = "texture2D";
    }
    else if (String_Equal(name, "tex2Dproj"))
    {
        name = "texture2DProj";
    }
    else if (String_Equal(name, "texCUBE"))
    {
        name = "textureCube";
    }
    else if (String_Equal(name, "clip"))
    {
        name = clipFunction;
    }
    else if (String_Equal(name, "tex2Dlod"))
    {
        name = "texture2DLod";
    }
    else if (String_Equal(name, "texCUBEbias"))
    {
        name = texCUBEbiasFunction;
    }
    else if (String_Equal(name, "atan2"))
    {
        name = "atan";
    }
    else if (String_Equal(name, "sincos"))
    {
        name = sinCosFunction;
    }
    else if (String_Equal(name, "fmod"))
    {
        // mod is not the same as fmod if the parameter is negative!
        // The equivalent of fmod(x, y) is x - y * floor(x/y)
        // We use the mod version for performance.
        name = "mod";
    }
    else if (String_Equal(name, "lerp"))
    {
        #if defined(__DAVAENGINE_MACOS__)
        name = "lerp";
        #else
        name = "mix";
        #endif
    }
    else if (String_Equal(name, "frac"))
    {
        name = "fract";
    }
    else
    {
        // The identifier could be a GLSL reserved word (if it's not also a HLSL reserved word).
        name = GetSafeIdentifierName(name);
    }
    writer.Write("%s", name);
}

void GLESGenerator::OutputArguments(HLSLArgument* argument)
{
    int numArgs = 0;
    while (argument != NULL)
    {
        if (numArgs > 0)
        {
            writer.Write(", ");
        }

        switch (argument->modifier)
        {
        case HLSLArgumentModifier_In:
            writer.Write("in ");
            break;
        case HLSLArgumentModifier_Inout:
            writer.Write("inout ");
            break;
        default:
            break; // to shut up goddamn warning
        }

        OutputDeclaration(argument->type, argument->name);
        argument = argument->nextArgument;
        ++numArgs;
    }
}

void GLESGenerator::OutputStatements(int indent, HLSLStatement* statement, const HLSLType* returnType)
{
    while (statement != NULL)
    {
        if (statement->nodeType == HLSLNodeType_Declaration)
        {
            HLSLDeclaration* declaration = static_cast<HLSLDeclaration*>(statement);

            if (strcmp(declaration->name, "output") != 0)
            {
                // GLSL doesn't seem have texture uniforms, so just ignore them.
                if (declaration->type.baseType != HLSLBaseType_Texture)
                {
                    writer.BeginLine(indent, declaration->fileName, declaration->line);
                    if (indent == 0)
                    {
                        // At the top level, we need the "uniform" keyword
                        if (declaration->type.flags & HLSLTypeFlag_Const)
                        {
                            writer.Write("const ");
                        }
                        else
                        {
                            if (!(declaration->type.flags & HLSLTypeFlag_Property))
                                writer.Write("uniform ");
                        }
                    }
                    OutputDeclaration(declaration);

                    if (declaration->type.flags & HLSLTypeFlag_Property)
                        writer.EndLine("");
                    else
                        writer.EndLine(";");
                }
            }
        }
        else if (statement->nodeType == HLSLNodeType_Struct)
        {
            HLSLStruct* structure = static_cast<HLSLStruct*>(statement);

            if (structure->usage == struct_Generic)
            {
                writer.WriteLine(indent, "struct");
                writer.WriteLine(indent, "%s", structure->name);
                writer.WriteLine(indent, "{");
                HLSLStructField* field = structure->field;
                while (field != NULL)
                {
                    writer.BeginLine(indent + 1, field->fileName, field->line);
                    OutputDeclaration(field->type, field->name);
                    writer.Write(";");
                    writer.EndLine();
                    field = field->nextField;
                }
                writer.WriteLine(indent, "};");
            }
        }
        else if (statement->nodeType == HLSLNodeType_Buffer)
        {
            HLSLBuffer* buffer = static_cast<HLSLBuffer*>(statement);
            HLSLLiteralExpression* arr_sz = static_cast<HLSLLiteralExpression*>(buffer->field->type.arraySize);
            HLSLDeclaration* decl = buffer->field;
            bool isBigArray = decl->annotation && strstr(decl->annotation, "bigarray");

            writer.WriteLine
            (
            indent, "uniform %s %s[%i];",
            GetTypeName(decl->type),
            (isBigArray) ? decl->registerName : decl->name,
            arr_sz->iValue
            );

            if (isBigArray)
                writer.WriteLine(indent, "#define %s %s", decl->name, decl->registerName);

            /*
            // Empty uniform blocks cause compilation errors on NVIDIA, so don't emit them.
            if (buffer->field != NULL)
            {
                m_writer.WriteLine(indent, buffer->fileName, buffer->line, "layout (std140) uniform %s {", buffer->name);
                HLSLDeclaration* field = buffer->field;
                while (field != NULL)
                {
                    m_writer.BeginLine(indent + 1, field->fileName, field->line);
                    OutputDeclaration(field->type, field->name);
                    m_writer.Write(";");
                    m_writer.EndLine();
                    field = (HLSLDeclaration*)field->nextStatement;
                }
                m_writer.WriteLine(indent, "};");
            }
*/
        }
        else if (statement->nodeType == HLSLNodeType_Function)
        {
            HLSLFunction* function = static_cast<HLSLFunction*>(statement);

            // Check if this is our entry point.
            bool entryPoint = String_Equal(function->name, entryName);

            if (!entryPoint)
            {
                // Use an alternate name for the function which is supposed to be entry point
                // so that we can supply our own function which will be the actual entry point.
                const char* functionName = GetSafeIdentifierName(function->name);
                const char* returnTypeName = GetTypeName(function->returnType);

                writer.BeginLine(indent, function->fileName, function->line);
                writer.Write("%s %s(", returnTypeName, functionName);

                OutputArguments(function->argument);

                writer.Write(") {");
                writer.EndLine();

                OutputStatements(indent + 1, function->statement, &function->returnType);
                writer.WriteLine(indent, "}");
            }
        }
        else if (statement->nodeType == HLSLNodeType_ExpressionStatement)
        {
            HLSLExpressionStatement* expressionStatement = static_cast<HLSLExpressionStatement*>(statement);
            writer.BeginLine(indent, statement->fileName, statement->line);
            OutputExpression(expressionStatement->expression);
            writer.EndLine(";");
        }
        else if (statement->nodeType == HLSLNodeType_ReturnStatement)
        {
            HLSLReturnStatement* returnStatement = static_cast<HLSLReturnStatement*>(statement);
            if (returnStatement->expression != NULL)
            {
                bool do_out_expr = true;

                if (returnStatement->expression->nodeType == HLSLNodeType_IdentifierExpression)
                {
                    HLSLIdentifierExpression* id = static_cast<HLSLIdentifierExpression*>(returnStatement->expression);

                    if (strcmp(id->name, "output") == 0)
                        do_out_expr = false;
                }

                if (do_out_expr)
                {
                    writer.BeginLine(indent, returnStatement->fileName, returnStatement->line);
                    writer.Write("return ");
                    OutputExpression(returnStatement->expression, returnType);
                    writer.EndLine(";");
                }
            }
            else
            {
                writer.WriteLine(indent, returnStatement->fileName, returnStatement->line, "return;");
            }
        }
        else if (statement->nodeType == HLSLNodeType_DiscardStatement)
        {
            HLSLDiscardStatement* discardStatement = static_cast<HLSLDiscardStatement*>(statement);
            if (target == TARGET_FRAGMENT)
            {
                writer.WriteLine(indent, discardStatement->fileName, discardStatement->line, "discard;");
            }
        }
        else if (statement->nodeType == HLSLNodeType_BreakStatement)
        {
            HLSLBreakStatement* breakStatement = static_cast<HLSLBreakStatement*>(statement);
            writer.WriteLine(indent, breakStatement->fileName, breakStatement->line, "break;");
        }
        else if (statement->nodeType == HLSLNodeType_ContinueStatement)
        {
            HLSLContinueStatement* continueStatement = static_cast<HLSLContinueStatement*>(statement);
            writer.WriteLine(indent, continueStatement->fileName, continueStatement->line, "continue;");
        }
        else if (statement->nodeType == HLSLNodeType_IfStatement)
        {
            HLSLIfStatement* ifStatement = static_cast<HLSLIfStatement*>(statement);
            writer.BeginLine(indent, ifStatement->fileName, ifStatement->line);
            writer.Write("if (");
            OutputExpression(ifStatement->condition, &kBoolType);
            writer.Write(") {");
            writer.EndLine();
            OutputStatements(indent + 1, ifStatement->statement, returnType);
            writer.WriteLine(indent, "}");
            if (ifStatement->elseStatement != NULL)
            {
                writer.WriteLine(indent, "else {");
                OutputStatements(indent + 1, ifStatement->elseStatement, returnType);
                writer.WriteLine(indent, "}");
            }
        }
        else if (statement->nodeType == HLSLNodeType_ForStatement)
        {
            HLSLForStatement* forStatement = static_cast<HLSLForStatement*>(statement);
            writer.BeginLine(indent, forStatement->fileName, forStatement->line);
            writer.Write("for (");
            OutputDeclaration(forStatement->initialization);
            writer.Write("; ");
            OutputExpression(forStatement->condition, &kBoolType);
            writer.Write("; ");
            OutputExpression(forStatement->increment);
            writer.Write(") {");
            writer.EndLine();
            OutputStatements(indent + 1, forStatement->statement, returnType);
            writer.WriteLine(indent, "}");
        }
        else if (statement->nodeType == HLSLNodeType_BlockStatement)
        {
            HLSLBlockStatement* blockStatement = static_cast<HLSLBlockStatement*>(statement);
            writer.WriteLine(indent, blockStatement->fileName, blockStatement->line, "{");
            OutputStatements(indent + 1, blockStatement->statement);
            writer.WriteLine(indent, "}");
        }
        else
        {
            // Unhanded statement type.
            DVASSERT(0);
        }

        statement = statement->nextStatement;
    }
}

HLSLFunction* GLESGenerator::FindFunction(HLSLRoot* root, const char* name)
{
    HLSLStatement* statement = root->statement;
    while (statement != NULL)
    {
        if (statement->nodeType == HLSLNodeType_Function)
        {
            HLSLFunction* function = static_cast<HLSLFunction*>(statement);
            if (String_Equal(function->name, name))
            {
                return function;
            }
        }
        statement = statement->nextStatement;
    }
    return NULL;
}

HLSLStruct* GLESGenerator::FindStruct(HLSLRoot* root, const char* name)
{
    HLSLStatement* statement = root->statement;
    while (statement != NULL)
    {
        if (statement->nodeType == HLSLNodeType_Struct)
        {
            HLSLStruct* structDeclaration = static_cast<HLSLStruct*>(statement);
            if (String_Equal(structDeclaration->name, name))
            {
                return structDeclaration;
            }
        }
        statement = statement->nextStatement;
    }
    return NULL;
}

void GLESGenerator::OutputAttribute(const HLSLType& type, const char* semantic, const char* attribType, const char* prefix)
{
    HLSLRoot* root = tree->GetRoot();
    if (type.baseType == HLSLBaseType_UserDefined)
    {
        // If the argument is a struct with semantics specified, we need to
        // grab them.
        HLSLStruct* structDeclaration = FindStruct(root, type.typeName);
        DVASSERT(structDeclaration != NULL);
        HLSLStructField* field = structDeclaration->field;
        while (field != NULL)
        {
            if (field->semantic != NULL && GetBuiltInSemantic(field->semantic) == NULL)
            {
                const char* typeName = GetTypeName(field->type);
                writer.WriteLine(0, "%s %s %s%s;", attribType, typeName, prefix, field->semantic);
            }
            field = field->nextField;
        }
    }
    else if (semantic != NULL && GetBuiltInSemantic(semantic) == NULL)
    {
        const char* typeName = GetTypeName(type);
        writer.WriteLine(0, "%s %s %s%s;", attribType, typeName, prefix, semantic);
    }
}

void GLESGenerator::OutputAttributes(HLSLFunction* entryFunction)
{
    // Write out the input attributes to the shader.
    HLSLArgument* argument = entryFunction->argument;
    while (argument != NULL)
    {
        OutputAttribute(argument->type, argument->semantic, "in", inAttribPrefix);
        argument = argument->nextArgument;
    }

    // Write out the output attributes from the shader.
    OutputAttribute(entryFunction->returnType, entryFunction->semantic, "out", outAttribPrefix);
}

void GLESGenerator::OutputSetOutAttribute(const char* semantic, const char* resultName)
{
    const char* builtInSemantic = GetBuiltInSemantic(semantic);
    if (builtInSemantic != NULL)
    {
        if (String_Equal(builtInSemantic, "gl_Position"))
        {
            // Mirror the y-coordinate when we're outputing from
            // the vertex shader so that we match the D3D texture
            // coordinate origin convention in render-to-texture
            // operations.
            // We also need to convert the normalized device
            // coordinates from the D3D convention of 0 to 1 to the
            // OpenGL convention of -1 to 1.
            writer.WriteLine(1, "vec4 temp = %s;", resultName);
            writer.WriteLine(1, "%s = temp * vec4(1,-1,2,1) - vec4(0,0,temp.w,0);", builtInSemantic);
            outputPosition = true;
        }
        else if (String_Equal(builtInSemantic, "gl_FragDepth"))
        {
            // If the value goes outside of the 0 to 1 range, the
            // fragment will be rejected unlike in D3D, so clamp it.
            writer.WriteLine(1, "%s = clamp(float(%s), 0.0, 1.0);", builtInSemantic, resultName);
        }
        else
        {
            writer.WriteLine(1, "%s = %s;", builtInSemantic, resultName);
        }
    }
    else
    {
        writer.WriteLine(1, "%s%s = %s;", outAttribPrefix, semantic, resultName);
    }
}

void GLESGenerator::OutputDeclaration(HLSLDeclaration* declaration)
{
    if (declaration->type.flags & HLSLTypeFlag_Property)
    {
        if (declaration->assignment)
        {
            writer.Write("#define %s ", declaration->name);
            writer.Write("(");
            if (declaration->type.array)
            {
                writer.Write("%s[]( ", GetTypeName(declaration->type));
                OutputExpressionList(declaration->assignment);
                writer.Write(" )");
            }
            else
            {
                OutputExpression(declaration->assignment, NULL /*&declaration->type*/);
            }
            writer.Write(")");
        }
    }
    else if (IsSamplerType(declaration->type))
    {
        int reg = -1;
        if (declaration->registerName != NULL)
        {
            sscanf(declaration->registerName, "s%d", &reg);
        }
        DVASSERT(reg != -1);

        const char* samplerType = NULL;

        if (declaration->type.baseType == HLSLBaseType_Sampler2D)
        {
            #if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_ANDROID__)
            samplerType = "lowp sampler2D";
            #else
            samplerType = "sampler2D";
            #endif
        }
        else if (declaration->type.baseType == HLSLBaseType_SamplerCube)
        {
            #if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_ANDROID__)
            samplerType = "lowp samplerCube";
            #else
            samplerType = "samplerCube";
            #endif
        }
        else if (declaration->type.baseType == HLSLBaseType_Sampler2DShadow)
        {
            #if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_ANDROID__)
            samplerType = "lowp sampler2DShadow";
            #else
            samplerType = "sampler2DShadow";
            #endif
        }

        const char* tname = (tree->FindGlobalStruct("vertex_in")) ? "Vertex" : "Fragment";

        writer.Write("%s %sTexture%d", samplerType, tname, reg);
    }
    else
    {
        OutputDeclaration(declaration->type, GetSafeIdentifierName(declaration->name));
        if (declaration->assignment != NULL)
        {
            writer.Write(" = ");
            if (declaration->type.array)
            {
                writer.Write("%s[]( ", GetTypeName(declaration->type));
                OutputExpressionList(declaration->assignment);
                writer.Write(" )");
            }
            else
            {
                OutputExpression(declaration->assignment, &declaration->type);
            }
        }
    }
}

void GLESGenerator::OutputDeclaration(const HLSLType& type, const char* name)
{
    if (!type.array)
    {
        writer.Write("%s %s", GetTypeName(type), GetSafeIdentifierName(name));
    }
    else
    {
        writer.Write("%s %s[", GetTypeName(type), GetSafeIdentifierName(name));
        if (type.arraySize != NULL)
        {
            OutputExpression(type.arraySize);
        }
        writer.Write("]");
    }
}

void GLESGenerator::Error(const char* format, ...)
{
    // It's not always convenient to stop executing when an error occurs,
    // so just track once we've hit an error and stop reporting them until
    // we successfully bail out of execution.
    if (hasError)
    {
        return;
    }
    hasError = true;

    char buffer[1024];
    va_list args;
    va_start(args, format);
    int result = vsnprintf(buffer, sizeof(buffer) - 1, format, args);
    va_end(args);

    DAVA::Logger::Error(buffer);
}

const char* GLESGenerator::GetSafeIdentifierName(const char* name) const
{
    for (int i = 0; i < NumReservedWords; ++i)
    {
        if (String_Equal(ReservedWord[i], name))
        {
            return reservedWord[i];
        }
    }
    return name;
}

bool GLESGenerator::ChooseUniqueName(const char* base, char* dst, int dstLength) const
{
    for (int i = 0; i < 1024; ++i)
    {
        Snprintf(dst, dstLength, "%s%d", base, i);
        if (!tree->GetContainsString(dst))
        {
            return true;
        }
    }
    return false;
}

} // namespace sl
