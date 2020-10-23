#include "sl_Common.h"

#include "sl_GeneratorMSL.h"
#include "sl_Parser.h"
#include "sl_Tree.h"

#include "Render/RHI/rhi_Type.h"
#include "Render/RHI/Common/rhi_Utils.h"

namespace sl
{
const char* MSLGenerator::GetTypeName(const HLSLType& type)
{
    switch (type.baseType)
    {
    case HLSLBaseType_Void:
        return "void";
    case HLSLBaseType_Float:
        return "float";
    case HLSLBaseType_Float2:
        return "vector_float2";
    case HLSLBaseType_Float3:
        return "vector_float3";
    case HLSLBaseType_Float4:
        return "vector_float4";
    case HLSLBaseType_Float3x3:
        return "float3x3";
    case HLSLBaseType_Float4x4:
        return "float4x4";
    case HLSLBaseType_Half:
        return "half";
    case HLSLBaseType_Half2:
        return "half2";
    case HLSLBaseType_Half3:
        return "half3";
    case HLSLBaseType_Half4:
        return "half4";
    case HLSLBaseType_Half3x3:
        return "half3x3";
    case HLSLBaseType_Half4x4:
        return "half4x4";
    case HLSLBaseType_Bool:
        return "bool";
    case HLSLBaseType_Int:
        return "int";
    case HLSLBaseType_Int2:
        return "int2";
    case HLSLBaseType_Int3:
        return "int3";
    case HLSLBaseType_Int4:
        return "int4";
    case HLSLBaseType_Uint:
        return "uint";
    case HLSLBaseType_Uint2:
        return "uint2";
    case HLSLBaseType_Uint3:
        return "uint3";
    case HLSLBaseType_Uint4:
        return "uint4";
    case HLSLBaseType_Texture:
        return "texture";
    case HLSLBaseType_Sampler:
        return "sampler";
    case HLSLBaseType_Sampler2D:
        return "sampler2D";
    case HLSLBaseType_Sampler3D:
        return "sampler3D";
    case HLSLBaseType_SamplerCube:
        return "samplerCUBE";
    case HLSLBaseType_Sampler2DShadow:
        return "sampler2DShadow";
    case HLSLBaseType_Sampler2DMS:
        return "sampler2DMS";
    case HLSLBaseType_UserDefined:
        return type.typeName;
    default:
        break; // to shut up goddamn warning
    }
    return "?";
}
/*
static int GetFunctionArguments(HLSLFunctionCall* functionCall, HLSLExpression* expression[], int maxArguments)
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
*/
MSLGenerator::MSLGenerator(Allocator* allocator)
{
    tree = NULL;
    entryName = NULL;
    target = TARGET_VERTEX;
    isInsideBuffer = false;
}

// @@ We need a better way of doing semantic replacement:
// - Look at the function being generated.
// - Return semantic, semantics associated to fields of the return structure, or output arguments, or fields of structures associated to output arguments -> output semantic replacement.
// - Semantics associated input arguments or fields of the input arguments -> input semantic replacement.
/*
static const char* TranslateSemantic(const char* semantic, bool output, Target target)
{
    if (target == TARGET_VERTEX)
    {
        if (output)
        {
            if (String_Equal("POSITION", semantic))
                return "SV_Position";
        }
    }
    else if (target == TARGET_FRAGMENT)
    {
        if (output)
        {
            if (String_Equal("DEPTH", semantic))
                return "SV_Depth";
            if (String_Equal("COLOR", semantic))
                return "SV_Target";
            if (String_Equal("COLOR0", semantic))
                return "SV_Target0";
            if (String_Equal("COLOR1", semantic))
                return "SV_Target1";
            if (String_Equal("COLOR2", semantic))
                return "SV_Target2";
            if (String_Equal("COLOR3", semantic))
                return "SV_Target3";
        }
        else
        {
            if (String_Equal("VPOS", semantic))
                return "SV_Position";
            if (String_Equal("VFACE", semantic))
                return "SV_IsFrontFace"; // bool   @@ Should we do type replacement too?
        }
    }
    return NULL;
}
*/
bool MSLGenerator::Generate(HLSLTree* tree_, Target target_, const char* entryName_, std::string* code)
{
    tree = tree_;
    entryName = entryName_;
    target = target_;
    isInsideBuffer = false;

    writer.Reset(code);
    texInfo.clear();

    const char* mtl_define[] =
    {

      "#include <metal_stdlib>",
      "#include <metal_graphics>",
      "#include <metal_matrix>",
      "#include <metal_geometric>",
      "#include <metal_math>",
      "#include <metal_texture>",
      "using namespace metal;",

      "#define lerp(a,b,t) mix( (a), (b), (t) )",
      "#define  frac(a) fract(a)",

      "inline vector_float4 mul( vector_float4 v, float4x4 m ) { return m*v; }",
      "inline vector_float4 mul( float4x4 m, vector_float4 v ) { return v*m; }",
      "inline vector_float3 mul( vector_float3 v, float3x3 m ) { return m*v; }",

      "#define FP_A8(t) (t).a",

      ""
    };

    for (unsigned i = 0; i != countof(mtl_define); ++i)
        writer.WriteLine(0, mtl_define[i]);

    HLSLRoot* root = tree->GetRoot();
    OutputStatements(0, root->statement);

    tree = NULL;
    return true;
}

const char* MSLGenerator::GetResult() const
{
    return writer.GetResult();
}

void MSLGenerator::OutputExpressionList(HLSLExpression* expression)
{
    int numExpressions = 0;
    while (expression != NULL)
    {
        if (numExpressions > 0)
        {
            writer.Write(", ");
        }
        OutputExpression(expression);
        expression = expression->nextExpression;
        ++numExpressions;
    }
}

void MSLGenerator::OutputExpression(HLSLExpression* expression)
{
    if (expression->nodeType == HLSLNodeType_IdentifierExpression)
    {
        HLSLIdentifierExpression* identifierExpression = static_cast<HLSLIdentifierExpression*>(expression);
        const char* name = identifierExpression->name;

        if (IsSamplerType(identifierExpression->expressionType) && identifierExpression->global)
        {
            if (identifierExpression->expressionType.baseType == HLSLBaseType_Sampler2D || identifierExpression->expressionType.baseType == HLSLBaseType_SamplerCube)
            {
                writer.Write("%s_texture.sample( %s_sampler ", name, name);
            }
        }
        else
        {
            writer.Write("%s", name);
        }
    }
    else if (expression->nodeType == HLSLNodeType_CastingExpression)
    {
        HLSLCastingExpression* castingExpression = static_cast<HLSLCastingExpression*>(expression);
        const char* tname = GetTypeName(castingExpression->type);

        if (strstr(tname, "vector_float"))
        {
            OutputDeclaration(castingExpression->type, "");
            writer.Write("(");
            OutputExpression(castingExpression->expression);
            writer.Write(")");
        }
        else
        {
            writer.Write("(");
            OutputDeclaration(castingExpression->type, "");
            writer.Write(")(");
            OutputExpression(castingExpression->expression);
            writer.Write(")");
        }
    }
    else if (expression->nodeType == HLSLNodeType_ConstructorExpression)
    {
        HLSLConstructorExpression* constructorExpression = static_cast<HLSLConstructorExpression*>(expression);
        writer.Write("%s(", GetTypeName(constructorExpression->type));
        OutputExpressionList(constructorExpression->argument);
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
            OutputExpression(unaryExpression->expression);
        }
        else
        {
            OutputExpression(unaryExpression->expression);
            writer.Write("%s", op);
        }
        writer.Write(")");
    }
    else if (expression->nodeType == HLSLNodeType_BinaryExpression)
    {
        HLSLBinaryExpression* binaryExpression = static_cast<HLSLBinaryExpression*>(expression);
        writer.Write("(");
        OutputExpression(binaryExpression->expression1);
        const char* op = "?";
        switch (binaryExpression->binaryOp)
        {
        case HLSLBinaryOp_Add:
            op = " + ";
            break;
        case HLSLBinaryOp_Sub:
            op = " - ";
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
            break;
        case HLSLBinaryOp_AddAssign:
            op = " += ";
            break;
        case HLSLBinaryOp_SubAssign:
            op = " -= ";
            break;
        case HLSLBinaryOp_MulAssign:
            op = " *= ";
            break;
        case HLSLBinaryOp_DivAssign:
            op = " /= ";
            break;
        case HLSLBinaryOp_And:
            op = " && ";
            break;
        case HLSLBinaryOp_Or:
            op = " || ";
            break;
        default:
            DVASSERT(0);
        }
        writer.Write("%s", op);
        OutputExpression(binaryExpression->expression2);
        writer.Write(")");
    }
    else if (expression->nodeType == HLSLNodeType_ConditionalExpression)
    {
        HLSLConditionalExpression* conditionalExpression = static_cast<HLSLConditionalExpression*>(expression);
        writer.Write("((");
        OutputExpression(conditionalExpression->condition);
        writer.Write(")?(");
        OutputExpression(conditionalExpression->trueExpression);
        writer.Write("):(");
        OutputExpression(conditionalExpression->falseExpression);
        writer.Write("))");
    }
    else if (expression->nodeType == HLSLNodeType_MemberAccess)
    {
        HLSLMemberAccess* memberAccess = static_cast<HLSLMemberAccess*>(expression);
#if 0        
        writer.Write("(");
        OutputExpression(memberAccess->object);
        writer.Write(").%s", memberAccess->field);
#else
        OutputExpression(memberAccess->object);
        writer.Write(".%s", memberAccess->field);
#endif
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
        const char* name = functionCall->function->name;
        bool sampler_call = false;
        bool sampler_lod = String_Equal(name, "tex2Dlod");
        bool sampler_cmp = String_Equal(name, "tex2Dcmp");
        bool sampler_prj = String_Equal(name, "tex2Dproj");

        if (String_Equal(name, "tex2D") || String_Equal(name, "tex2Dproj") || String_Equal(name, "tex2Dcmp") || String_Equal(name, "tex2Dlod") || String_Equal(name, "texCUBE"))
        {
            sampler_call = true;
        }

        if (sampler_call)
        {
            if (sampler_lod)
            {
                DVASSERT(functionCall->argument->nodeType == HLSLNodeType_IdentifierExpression);
                HLSLIdentifierExpression* identifier = static_cast<HLSLIdentifierExpression*>(functionCall->argument);
                DVASSERT(IsSamplerType(identifier->expressionType) && identifier->global);

                if (identifier->expressionType.baseType == HLSLBaseType_Sampler2D)
                {
                    writer.Write("%s_texture.sample( %s_sampler ", identifier->name, identifier->name);
                    int arg = 2;

                    for (HLSLExpression* expr = identifier->nextExpression; expr; expr = expr->nextExpression)
                    {
                        writer.Write(", ");

                        if (arg == 3)
                            writer.Write("level(");
                        OutputExpression(expr);
                        if (arg == 3)
                            writer.Write(")");

                        ++arg;
                    }
                    writer.Write(")");
                }
            }
            else if (sampler_prj)
            {
                DVASSERT(functionCall->argument->nodeType == HLSLNodeType_IdentifierExpression);
                HLSLIdentifierExpression* identifier = static_cast<HLSLIdentifierExpression*>(functionCall->argument);
                DVASSERT(IsSamplerType(identifier->expressionType) && identifier->global);

                HLSLExpression* expr = identifier->nextExpression;
                writer.Write("%s_texture.sample( %s_sampler, (", identifier->name, identifier->name);
                OutputExpression(expr);
                writer.Write(").xy / (");
                OutputExpression(expr);
                writer.Write(").w ");

                writer.Write(")");
            }
            else if (sampler_cmp)
            {
                DVASSERT(functionCall->argument->nodeType == HLSLNodeType_IdentifierExpression);
                HLSLIdentifierExpression* identifier = static_cast<HLSLIdentifierExpression*>(functionCall->argument);
                DVASSERT(IsSamplerType(identifier->expressionType) && identifier->global);

                if (identifier->expressionType.baseType == HLSLBaseType_Sampler2DShadow)
                {
                    HLSLExpression* expr = identifier->nextExpression;

                    writer.Write("%s_texture.sample_compare( %s_sampler ", identifier->name, identifier->name);
                    writer.Write(", ");
                    OutputExpression(expr);
                    writer.Write(".xy, ");
                    OutputExpression(expr);
                    writer.Write(".z, ");
                    writer.Write("level(0) ");
                    writer.Write(")");
                }
            }
            else
            {
                OutputExpressionList(functionCall->argument);
                writer.Write(")");
            }
        }
        else
        {
            writer.Write("%s(", name);
            OutputExpressionList(functionCall->argument);
            writer.Write(")");
        }
    }
    else
    {
        writer.Write("<unknown expression>");
    }
}

void MSLGenerator::OutputArguments(HLSLArgument* argument)
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
        case HLSLArgumentModifier_Out:
            writer.Write("out ");
            break;
        case HLSLArgumentModifier_Inout:
            writer.Write("inout ");
            break;
        case HLSLArgumentModifier_Uniform:
            writer.Write("uniform ");
            break;
        default:
            break; // to shut up goddamn warning
        }

        const char* semantic = argument->sv_semantic ? argument->sv_semantic : argument->semantic;

        OutputDeclaration(argument->type, argument->name, semantic, /*registerName=*/NULL, argument->defaultValue);
        argument = argument->nextArgument;
        ++numArgs;
    }
}

const char* MSLGenerator::GetAttributeName(HLSLAttributeType attributeType)
{
    if (attributeType == HLSLAttributeType_Unroll)
        return "unroll";
    if (attributeType == HLSLAttributeType_Branch)
        return "branch";
    if (attributeType == HLSLAttributeType_Flatten)
        return "flatten";
    return NULL;
}

void MSLGenerator::OutputAttributes(int indent, HLSLAttribute* attribute)
{
    while (attribute != NULL)
    {
        const char* attributeName = GetAttributeName(attribute->attributeType);

        if (attributeName != NULL)
        {
            writer.WriteLine(indent, attribute->fileName, attribute->line, "[%s]", attributeName);
        }

        attribute = attribute->nextAttribute;
    }
}

void MSLGenerator::OutputStatements(int indent, HLSLStatement* statement)
{
    while (statement != NULL)
    {
        if (statement->hidden)
        {
            statement = statement->nextStatement;
            continue;
        }

        OutputAttributes(indent, statement->attributes);

        if (statement->nodeType == HLSLNodeType_Declaration)
        {
            HLSLDeclaration* declaration = static_cast<HLSLDeclaration*>(statement);
            if (!(declaration->type.flags & HLSLTypeFlag_Property))
            {
                writer.BeginLine(indent, declaration->fileName, declaration->line);
                OutputDeclaration(declaration);
                writer.EndLine(";");
            }
        }
        else if (statement->nodeType == HLSLNodeType_Struct)
        {
            HLSLStruct* structure = static_cast<HLSLStruct*>(statement);
            writer.WriteLine(indent, structure->fileName, structure->line, "struct %s", structure->name);
            writer.WriteLine(indent, structure->fileName, structure->line, "{");
            HLSLStructField* field = structure->field;
            while (field != NULL)
            {
                if (!field->hidden)
                {
                    writer.BeginLine(indent + 1, field->fileName, field->line);
                    const char* semantic = field->sv_semantic ? field->sv_semantic : field->semantic;
                    char attr_name[64];

                    if (structure->usage == struct_VertexIn || structure->usage == struct_VertexOut || structure->usage == struct_FragmentIn || structure->usage == struct_FragmentOut)
                    {
                        struct
                        {
                            const char* sv_semantic;
                            unsigned vattr;
                            const char* mtl_semantic;
                            const char* mtl_fp_semantic;
                        } attr[] =
                        {
                          { "POSITION", rhi::VATTR_POSITION_0, "position0", "" },
                          { "POSITION0", rhi::VATTR_POSITION_1, "position0", "" },
                          { "POSITION1", rhi::VATTR_POSITION_1, "position1", "" },
                          { "POSITION2", rhi::VATTR_POSITION_2, "position2", "" },
                          { "POSITION3", rhi::VATTR_POSITION_3, "position3", "" },
                          { "SV_POSITION", rhi::VATTR_POSITION_0, "position", "" },
                          { "NORMAL", rhi::VATTR_NORMAL_0, "normal0", "" },
                          { "NORMAL0", rhi::VATTR_NORMAL_0, "normal0", "" },
                          { "NORMAL1", rhi::VATTR_NORMAL_1, "normal1", "" },
                          { "NORMAL2", rhi::VATTR_NORMAL_2, "normal2", "" },
                          { "NORMAL3", rhi::VATTR_NORMAL_3, "normal3", "" },
                          { "TEXCOORD", rhi::VATTR_TEXCOORD_0, "texcoord0", "" },
                          { "TEXCOORD0", rhi::VATTR_TEXCOORD_0, "texcoord0", "" },
                          { "TEXCOORD1", rhi::VATTR_TEXCOORD_1, "texcoord1", "" },
                          { "TEXCOORD2", rhi::VATTR_TEXCOORD_2, "texcoord2", "" },
                          { "TEXCOORD3", rhi::VATTR_TEXCOORD_3, "texcoord3", "" },
                          { "TEXCOORD4", rhi::VATTR_TEXCOORD_4, "texcoord4", "" },
                          { "TEXCOORD5", rhi::VATTR_TEXCOORD_5, "texcoord5", "" },
                          { "TEXCOORD6", rhi::VATTR_TEXCOORD_6, "texcoord6", "" },
                          { "TEXCOORD7", rhi::VATTR_TEXCOORD_7, "texcoord7", "" },
                          { "COLOR", rhi::VATTR_COLOR_0, "color0", "color(0)" },
                          { "COLOR0", rhi::VATTR_COLOR_0, "color0", "color(0)" },
                          { "COLOR1", rhi::VATTR_COLOR_1, "color1", "color(1)" },
                          { "SV_TARGET", rhi::VATTR_COLOR_0, "color0", "color(0)" },
                          { "SV_TARGET0", rhi::VATTR_COLOR_0, "color0", "color(0)" },
                          { "SV_TARGET1", rhi::VATTR_COLOR_1, "color1", "color(1)" },
                          { "SV_TARGET2", rhi::VATTR_COLOR_1, "color2", "color(2)" },
                          { "SV_TARGET3", rhi::VATTR_COLOR_1, "color3", "color(3)" },
                          { "TANGENT", rhi::VATTR_TANGENT, "tangent", "" },
                          { "BINORMAL", rhi::VATTR_BINORMAL, "binormal", "" },
                          { "BLENDWEIGHT", rhi::VATTR_BLENDWEIGHT, "blend_weight", "" },
                          { "BLENDINDICES", rhi::VATTR_BLENDINDEX, "blend_index", "" }
                        };

                        for (unsigned i = 0; i != countof(attr); ++i)
                        {
                            if (stricmp(attr[i].sv_semantic, semantic) == 0)
                            {
                                if (structure->usage == struct_VertexIn)
                                    Snprintf(attr_name, countof(attr_name), "[[ attribute(%u) ]]", attr[i].vattr);
                                else if (structure->usage == struct_VertexOut && stricmp(semantic, "SV_POSITION") == 0)
                                    Snprintf(attr_name, countof(attr_name), "[[ position ]]");
                                else if (structure->usage == struct_VertexOut || structure->usage == struct_FragmentIn)
                                    Snprintf(attr_name, countof(attr_name), "[[ user(%s) ]]", attr[i].mtl_semantic);
                                else if (structure->usage == struct_FragmentOut)
                                    Snprintf(attr_name, countof(attr_name), "[[ %s ]]", attr[i].mtl_fp_semantic);

                                semantic = attr_name;
                                break;
                            }
                        }
                    }

                    OutputDeclaration(field->type, field->name, semantic);
                    writer.Write(";");
                    writer.EndLine();
                }
                field = field->nextField;
            }
            writer.WriteLine(indent, "};");
        }
        else if (statement->nodeType == HLSLNodeType_Buffer)
        {
            HLSLBuffer* buffer = static_cast<HLSLBuffer*>(statement);
            HLSLLiteralExpression* arr_sz = static_cast<HLSLLiteralExpression*>(buffer->field->type.arraySize);

            writer.WriteLine
            (
            indent, buffer->fileName, buffer->line,
            "struct __%s { packed_float4 data[%i]; };",
            buffer->name,
            buffer->registerCount
            );
        }
        else if (statement->nodeType == HLSLNodeType_Function)
        {
            HLSLFunction* function = static_cast<HLSLFunction*>(statement);

            if (strcmp(function->name, entryName) == 0)
            {
                const char* returnTypeName = GetTypeName(function->returnType);
                char btype = (target == TARGET_VERTEX) ? 'V' : 'F';

                writer.BeginLine(indent, function->fileName, function->line);
                writer.WriteLine(indent, "%s %s %s", (target == TARGET_VERTEX) ? "vertex" : "fragment", returnTypeName, function->name);
                writer.WriteLine(indent, "(");

                writer.WriteLine(indent + 1, "%s in [[ stage_in ]]", GetTypeName(function->argument->type));
                for (unsigned i = 0; i != 32; ++i)
                {
                    char bname[64];

                    Snprintf(bname, countof(bname), "%cP_Buffer%u_t", btype, i);
                    if (tree->FindBuffer(bname))
                    {
                        writer.WriteLine(indent + 1, ", constant __%cP_Buffer%u_t* buf%u [[ buffer(%u) ]]", btype, i, i, (target == TARGET_VERTEX) ? rhi::MAX_VERTEX_STREAM_COUNT + i : i);
                    }
                }
                for (unsigned t = 0; t != texInfo.size(); ++t)
                {
                    const char* ttype = "";

                    if (texInfo[t].type == HLSLBaseType_Sampler2D)
                    {
                        ttype = "texture2d<float>";
                    }
                    else if (texInfo[t].type == HLSLBaseType_SamplerCube)
                    {
                        ttype = "texturecube<float>";
                    }

                    writer.WriteLine(indent + 1, ", %s %s_texture [[ texture(%u) ]]", ttype, texInfo[t].name.c_str(), texInfo[t].unit);
                }
                for (unsigned t = 0; t != texInfo.size(); ++t)
                {
                    writer.WriteLine(indent + 1, ", sampler %s_sampler [[ sampler(%u) ]]", texInfo[t].name.c_str(), texInfo[t].unit);
                }
                writer.WriteLine(indent, ")");
                writer.WriteLine(indent, "{");
                for (unsigned i = 0; i != 32; ++i)
                {
                    char bname[64];

                    Snprintf(bname, countof(bname), "%cP_Buffer%u_t", btype, i);
                    if (tree->FindBuffer(bname))
                    {
                        writer.WriteLine(indent + 1, "constant packed_float4* %cP_Buffer%u = buf%u->data;", btype, i, i);
                    }
                }
                writer.WriteLine(indent + 1, "%s %s = in;", GetTypeName(function->argument->type), function->argument->name);

                for (HLSLStatement* s = tree->GetRoot()->statement; s; s = s->nextStatement)
                {
                    if (s->nodeType == HLSLNodeType_Declaration)
                    {
                        HLSLDeclaration* decl = static_cast<HLSLDeclaration*>(s);

                        if (decl->type.flags & HLSLTypeFlag_Property)
                        {
                            writer.BeginLine(indent + 1);
                            if (decl->type.array)
                            {
                                writer.Write("constant vector_float4* %s = (constant vector_float4*)%s", decl->name, decl->registerName);
                            }
                            else
                            {
                                OutputDeclarationType(decl->type);
                                OutputDeclarationBody(decl->type, decl->name, decl->semantic, decl->registerName, decl->assignment);
                            }
                            writer.Write(";");
                            writer.EndLine();
                        }
                    }
                }

                OutputStatements(indent + 1, function->statement);
                writer.WriteLine(indent, "};");
            }
            else
            {
                // Use an alternate name for the function which is supposed to be entry point
                // so that we can supply our own function which will be the actual entry point.
                const char* functionName = function->name;
                const char* returnTypeName = GetTypeName(function->returnType);

                writer.BeginLine(indent, function->fileName, function->line);
                writer.Write("inline %s %s(", returnTypeName, functionName);

                OutputArguments(function->argument);

                const char* semantic = function->sv_semantic ? function->sv_semantic : function->semantic;
                if (semantic != NULL)
                {
                    writer.Write(") : %s {", semantic);
                }
                else
                {
                    writer.Write(") {");
                }

                writer.EndLine();

                OutputStatements(indent + 1, function->statement);
                writer.WriteLine(indent, "};");
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
                writer.BeginLine(indent, returnStatement->fileName, returnStatement->line);
                writer.Write("return ");
                OutputExpression(returnStatement->expression);
                writer.EndLine(";");
            }
            else
            {
                writer.WriteLine(indent, returnStatement->fileName, returnStatement->line, "return;");
            }
        }
        else if (statement->nodeType == HLSLNodeType_DiscardStatement)
        {
            HLSLDiscardStatement* discardStatement = static_cast<HLSLDiscardStatement*>(statement);
            writer.WriteLine(indent, discardStatement->fileName, discardStatement->line, "discard_fragment();");
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
            OutputExpression(ifStatement->condition);
            writer.Write(") {");
            writer.EndLine();
            OutputStatements(indent + 1, ifStatement->statement);
            writer.WriteLine(indent, "}");
            if (ifStatement->elseStatement != NULL)
            {
                writer.WriteLine(indent, "else {");
                OutputStatements(indent + 1, ifStatement->elseStatement);
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
            OutputExpression(forStatement->condition);
            writer.Write("; ");
            OutputExpression(forStatement->increment);
            writer.Write(") {");
            writer.EndLine();
            OutputStatements(indent + 1, forStatement->statement);
            writer.WriteLine(indent, "}");
        }
        else if (statement->nodeType == HLSLNodeType_BlockStatement)
        {
            HLSLBlockStatement* blockStatement = static_cast<HLSLBlockStatement*>(statement);
            writer.WriteLine(indent, blockStatement->fileName, blockStatement->line, "{");
            OutputStatements(indent + 1, blockStatement->statement);
            writer.WriteLine(indent, "}");
        }
        else if (statement->nodeType == HLSLNodeType_Technique)
        {
            // Techniques are ignored.
        }
        else if (statement->nodeType == HLSLNodeType_Pipeline)
        {
            // Pipelines are ignored.
        }
        else
        {
            // Unhanded statement type.
            DVASSERT(0);
        }

        statement = statement->nextStatement;
    }
}

void MSLGenerator::OutputDeclaration(HLSLDeclaration* declaration)
{
    bool isSamplerType = IsSamplerType(declaration->type);

    if (isSamplerType)
    {
        int reg = -1;
        if (declaration->registerName != NULL)
        {
            sscanf(declaration->registerName, "s%d", &reg);
        }

        const char* textureType = NULL;
        const char* samplerType = "SamplerState";
        // @@ Handle generic sampler type.

        if (declaration->type.baseType == HLSLBaseType_Sampler2D)
        {
            textureType = "texture2d<float>";
            samplerType = "sampler";
        }
        else if (declaration->type.baseType == HLSLBaseType_Sampler3D)
        {
            textureType = "Texture3D";
        }
        else if (declaration->type.baseType == HLSLBaseType_SamplerCube)
        {
            textureType = "TextureCube";
        }
        else if (declaration->type.baseType == HLSLBaseType_Sampler2DShadow)
        {
            textureType = "Texture2D";
            samplerType = "SamplerComparisonState";
        }
        else if (declaration->type.baseType == HLSLBaseType_Sampler2DMS)
        {
            textureType = "Texture2DMS<float4>"; // @@ Is template argument required?
            samplerType = NULL;
        }

        if (samplerType != NULL)
        {
            tex_t tex;

            tex.name = declaration->name;
            tex.type = declaration->type.baseType;
            tex.unit = unsigned(texInfo.size());
            texInfo.push_back(tex);
        }
        else
        {
            if (reg != -1)
            {
                writer.Write("%s %s : register(t%d)", textureType, declaration->name, reg);
            }
            else
            {
                writer.Write("%s %s", textureType, declaration->name);
            }
        }
        return;
    }

    OutputDeclarationType(declaration->type);
    OutputDeclarationBody(declaration->type, declaration->name, declaration->semantic, declaration->registerName, declaration->assignment);
    declaration = declaration->nextDeclaration;

    while (declaration != NULL)
    {
        writer.Write(", ");
        OutputDeclarationBody(declaration->type, declaration->name, declaration->semantic, declaration->registerName, declaration->assignment);
        declaration = declaration->nextDeclaration;
    };
}

void MSLGenerator::OutputDeclarationType(const HLSLType& type)
{
    const char* typeName = GetTypeName(type);

    if (type.flags & HLSLTypeFlag_Const)
    {
        writer.Write("const ");
    }
    if (type.flags & HLSLTypeFlag_Static && !(type.flags & HLSLTypeFlag_Property))
    {
        writer.Write("static ");
    }

    // Interpolation modifiers.
    if (type.flags & HLSLTypeFlag_Centroid)
    {
        writer.Write("centroid ");
    }
    if (type.flags & HLSLTypeFlag_Linear)
    {
        writer.Write("linear ");
    }
    if (type.flags & HLSLTypeFlag_NoInterpolation)
    {
        writer.Write("nointerpolation ");
    }
    if (type.flags & HLSLTypeFlag_NoPerspective)
    {
        writer.Write("noperspective ");
    }
    if (type.flags & HLSLTypeFlag_Sample) // @@ Only in shader model >= 4.1
    {
        writer.Write("sample ");
    }

    writer.Write("%s ", typeName);
}

void MSLGenerator::OutputDeclarationBody(const HLSLType& type, const char* name, const char* semantic /*=NULL*/, const char* registerName /*=NULL*/, HLSLExpression* assignment /*=NULL*/)
{
    writer.Write("%s", name);

    if (type.array)
    {
        DVASSERT(semantic == NULL);
        writer.Write("[");
        if (type.arraySize != NULL)
        {
            OutputExpression(type.arraySize);
        }
        writer.Write("]");
    }

    if (semantic != NULL)
    {
        writer.Write(" %s", semantic);
    }

    if (registerName != NULL)
    {
        if (isInsideBuffer)
        {
            writer.Write(" : packoffset(%s)", registerName);
        }
        else
        {
            writer.Write(" : register(%s)", registerName);
        }
    }

    if (assignment != NULL && !IsSamplerType(type))
    {
        writer.Write(" = ");
        if (type.array)
        {
            writer.Write("{ ");
            OutputExpressionList(assignment);
            writer.Write(" }");
        }
        else
        {
            OutputExpression(assignment);
        }
    }
}

void MSLGenerator::OutputDeclaration(const HLSLType& type, const char* name, const char* semantic /*=NULL*/, const char* registerName /*=NULL*/, HLSLExpression* assignment /*=NULL*/)
{
    OutputDeclarationType(type);
    OutputDeclarationBody(type, name, semantic, registerName, assignment);
}

} // namespace sl