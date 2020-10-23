#include "Render/RHI/rhi_ShaderSource.h"
#include "Render/RHI/rhi_Public.h"
#include "Render/RHI/Common/rhi_Utils.h"
#include "Render/RHI/Common/PreProcessor.h"


#include "Logger/Logger.h"
using DAVA::Logger;
#include "FileSystem/DynamicMemoryFile.h"
#include "FileSystem/FileSystem.h"
using DAVA::DynamicMemoryFile;
#include "Utils/Utils.h"
#include "Utils/StringFormat.h"
#include "Debug/ProfilerCPU.h"
#include "Concurrency/Mutex.h"
#include "Concurrency/LockGuard.h"
using DAVA::Mutex;
using DAVA::LockGuard;

#include "Parser/sl_Parser.h"
#include "Parser/sl_Tree.h"
#include "Parser/sl_GeneratorHLSL.h"
#include "Parser/sl_GeneratorGLES.h"
#include "Parser/sl_GeneratorMSL.h"

#define RHI_DUMP_SHADERSOURCE 0

namespace rhi
{
//==============================================================================

class ShaderFileCallback : public DAVA::PreProc::FileCallback
{
public:
    ShaderFileCallback(const char* base_dir)
    {
        inclDir.emplace_back(base_dir);
    }

    ~ShaderFileCallback()
    {
        ClearCache();
    }

    bool Open(const char* file_name) override
    {
        bool success = false;

        for (size_t k = 0; k != _file.size(); ++k)
        {
            if (_file[k].name == file_name)
            {
                _cur_data = _file[k].data;
                _cur_data_sz = _file[k].data_sz;
                success = true;
                break;
            }
        }

        if (!success)
        {
            DAVA::File* in = nullptr;

            for (const std::string& d : inclDir)
            {
                in = DAVA::File::Create(d + "/" + file_name, DAVA::File::READ | DAVA::File::OPEN);

                if (in)
                    break;
            }

            if (in)
            {
                file_t f;

                f.name = file_name;
                f.data_sz = unsigned(in->GetSize());
                f.data = ::malloc(f.data_sz);

                in->Read(f.data, f.data_sz);
                in->Release();

                _file.push_back(f);
                _cur_data = f.data;
                _cur_data_sz = f.data_sz;

                success = true;
            }
        }

        return success;
    }

    void Close() override
    {
        _cur_data = nullptr;
        _cur_data_sz = 0;
    }

    unsigned Size() const override
    {
        return _cur_data_sz;
    }

    unsigned Read(unsigned max_sz, void* dst) override
    {
        DVASSERT(_cur_data);
        DVASSERT(max_sz <= _cur_data_sz);
        memcpy(dst, _cur_data, max_sz);
        return max_sz;
    }

    void AddIncludeDirectory(const char* dir)
    {
        inclDir.emplace_back(dir);
    }

    void ClearCache()
    {
        for (size_t k = 0; k != _file.size(); ++k)
        {
            ::free(_file[k].data);
        }
        _file.clear();
    }

private:
    struct file_t
    {
        std::string name;
        unsigned data_sz;
        void* data;
    };
    std::vector<file_t> _file;
    const void* _cur_data;
    unsigned _cur_data_sz;

    std::vector<std::string> inclDir;
};

static ShaderFileCallback ShaderSourceFileCallback("~res:/Materials/Shaders");

//==============================================================================

ShaderSource::ShaderSource(const char* filename)
    : fileName(filename)
    ,
    ast(nullptr)
{
}

//------------------------------------------------------------------------------

ShaderSource::~ShaderSource()
{
    delete ast;
    ast = nullptr;
}

//------------------------------------------------------------------------------

bool ShaderSource::Construct(ProgType progType, const char* srcText)
{
    std::vector<std::string> def;

    return ShaderSource::Construct(progType, srcText, def);
}

//------------------------------------------------------------------------------

bool ShaderSource::Construct(ProgType progType, const char* srcText, const std::vector<std::string>& defines)
{
    bool success = false;
    DAVA::PreProc pre_proc(&ShaderSourceFileCallback);
    std::vector<char> src;

    DVASSERT(defines.size() % 2 == 0);
    for (size_t i = 0, n = defines.size() / 2; i != n; ++i)
    {
        const char* name = defines[i * 2 + 0].c_str();
        const char* value = defines[i * 2 + 1].c_str();
        pre_proc.AddDefine(name, value);
    }

    if (pre_proc.Process(srcText, &src))
    {
        #if RHI_DUMP_SHADERSOURCE
        {
            Logger::Info("\n\nsrc-code:");

            char ss[64 * 1024];
            unsigned line_cnt = 0;

            if (src.size() < sizeof(ss))
            {
                strcpy(ss, &src[0]);

                const char* line = ss;
                for (char* s = ss; *s; ++s)
                {
                    if (*s == '\r')
                        *s = ' ';

                    if (*s == '\n')
                    {
                        *s = 0;
                        Logger::Info("%4u |  %s", 1 + line_cnt, line);
                        line = s + 1;
                        ++line_cnt;
                    }
                }
            }
            else
            {
                Logger::Info(&src[0]);
            }
        }
        #endif

        static sl::Allocator alloc;
        sl::HLSLParser parser(&alloc, "<shader>", src.data(), src.size());
        ast = new sl::HLSLTree(&alloc);

        if (parser.Parse(ast))
        {
            // some sanity checks
            const char* entryName = (progType == PROG_VERTEX) ? "vp_main" : "fp_main";
            bool hasReturn = false;
            sl::HLSLFunction* entryFunction = ast->FindFunction(entryName);
            if (entryFunction)
            {
                for (sl::HLSLStatement* s = entryFunction->statement; s; s = s->nextStatement)
                {
                    if (s->nodeType == sl::HLSLNodeType_ReturnStatement)
                    {
                        hasReturn = true;
                        break;
                    }
                }
            }
            else
            {
                DAVA::Logger::Error("missing entry-point function '%s'", entryName);
                return false;
            }

            if (!hasReturn)
            {
                DAVA::Logger::Error("entry-point function '%s' has no return statement", entryName);
                return false;
            }

            success = ProcessMetaData(ast);
            type = progType;

            if (success)
                InlineFunctions();

            // ugly workaround to save some memory
            GetSourceCode(HostApi());
            delete ast;
            ast = nullptr;
        }
        else
        {
            delete ast;
            ast = nullptr;
            DAVA::Logger::Error("failed to parse shader source-text");
        }
    }
    else
    {
        DAVA::Logger::Error("failed to pre-process source-text");
    }

    return success;
}
void ShaderSource::InlineFunctions()
{
    // first, find all global functions that are not entry-point
    std::vector<sl::HLSLFunction*> global_func_decl;

    for (sl::HLSLStatement* statement = ast->GetRoot()->statement; statement; statement = statement->nextStatement)
    {
        if (statement->nodeType == sl::HLSLNodeType_Function)
        {
            sl::HLSLFunction* function = static_cast<sl::HLSLFunction*>(statement);

            if (stricmp(function->name, "vp_main") && stricmp(function->name, "fp_main"))
                global_func_decl.push_back(function);
        }
    }

    // then, do the inlining
    for (unsigned i = 0; i != global_func_decl.size(); ++i)
    {
        // find function-call nodes
        sl::HLSLFunction* func_decl = global_func_decl[i];
        std::vector<sl::HLSLFunctionCall*> fcall;

        class FindFunctionCall : public sl::HLSLTreeVisitor
        {
        public:
            const char* name;
            std::vector<sl::HLSLFunctionCall*>* call;

            FindFunctionCall(const char* n, std::vector<sl::HLSLFunctionCall*>* c)
                : name(n)
                , call(c)
            {
            }
            virtual void VisitFunctionCall(sl::HLSLFunctionCall* node)
            {
                if (stricmp(node->function->name, name) == 0)
                    call->push_back(node);

                HLSLTreeVisitor::VisitFunctionCall(node);
            }
        };

        FindFunctionCall(func_decl->name, &fcall).VisitRoot(ast->GetRoot());

        class FindStatementExpression : public sl::HLSLTreeVisitor
        {
        public:
            FindStatementExpression(sl::HLSLFunctionCall* fc)
                : expr(nullptr)
                , statement(nullptr)
                , _fcall(fc)
                , _cur_statement(nullptr)
                , _cur_statement_parent(nullptr)
            {
            }

            sl::HLSLExpression* expr;
            sl::HLSLStatement* statement;

            virtual void VisitStatements(sl::HLSLStatement* statement)
            {
                _cur_statement = statement;
                HLSLTreeVisitor::VisitStatements(statement);
            }
            virtual void VisitDeclaration(sl::HLSLDeclaration* node)
            {
                _cur_statement = node;
                HLSLTreeVisitor::VisitDeclaration(node);
            }
            virtual void VisitExpressionStatement(sl::HLSLExpressionStatement* node)
            {
                _cur_statement = node;
                sl::HLSLTreeVisitor::VisitExpressionStatement(node);
            }
            virtual void VisitBlockStatement(sl::HLSLBlockStatement* node)
            {
                _cur_statement = node->statement;
                _cur_statement_parent = node;
                sl::HLSLTreeVisitor::VisitBlockStatement(node);
            }
            virtual void VisitExpression(sl::HLSLExpression* node)
            {
                _expr.push_back(node);
                sl::HLSLTreeVisitor::VisitExpression(node);
                if (_expr.size())
                    _expr.resize(_expr.size() - 1);
            }
            virtual void VisitFunctionCall(sl::HLSLFunctionCall* node)
            {
                if (node == _fcall)
                {
                    if (_expr.size() > 1)
                        expr = _expr[_expr.size() - 2];
                    statement = _cur_statement;
                }
                HLSLTreeVisitor::VisitFunctionCall(node);
            }

        private:
            std::vector<sl::HLSLExpression*> _expr;
            std::vector<sl::HLSLExpression**> _pexpr;

            sl::HLSLFunctionCall* _fcall;
            sl::HLSLStatement* _cur_statement;
            sl::HLSLStatement* _cur_statement_parent;
        };

        bool keep_func_def = false;
        if (fcall.size() > 1)
        {
            // can't inline function if it's called more than once
            fcall.clear();
            keep_func_def = true;
        }

        for (unsigned k = 0; k != fcall.size(); ++k)
        {
            FindStatementExpression find_statement_expression(fcall[k]);

            find_statement_expression.VisitRoot(ast->GetRoot());

            if (find_statement_expression.statement)
            {
                class FindParentPrev : public sl::HLSLTreeVisitor
                {
                public:
                    sl::HLSLStatement* target;

                    sl::HLSLStatement* parent;
                    sl::HLSLStatement* prev;

                    FindParentPrev()
                        : target(nullptr)
                        , parent(nullptr)
                        , prev(nullptr)
                    {
                    }

                    virtual void VisitFunction(sl::HLSLFunction* func)
                    {
                        sl::HLSLTreeVisitor::VisitFunction(func);
                    }
                    virtual void VisitStatements(sl::HLSLStatement* statement)
                    {
                        for (sl::HLSLStatement* s = statement; s; s = s->nextStatement)
                        {
                            if (s->nextStatement == target)
                            {
                                prev = s;
                                break;
                            }
                        }

                        switch (statement->nodeType)
                        {
                        case sl::HLSLNodeType_BlockStatement:
                        {
                            sl::HLSLBlockStatement* block = static_cast<sl::HLSLBlockStatement*>(statement);

                            if (block->statement == target)
                            {
                                parent = statement;
                            }
                            else
                            {
                                for (sl::HLSLStatement* bs = block->statement; bs; bs = bs->nextStatement)
                                {
                                    if (bs->nextStatement == target)
                                    {
                                        parent = block;
                                        prev = bs;
                                        break;
                                    }
                                }
                            }
                        }
                        break;
                        default:
                            break; // to shut up goddamn warning
                        }

                        sl::HLSLTreeVisitor::VisitStatements(statement);
                    }
                    virtual void VisitBlockStatement(sl::HLSLBlockStatement* node)
                    {
                        sl::HLSLBlockStatement* block = static_cast<sl::HLSLBlockStatement*>(node);

                        if (block->statement == target)
                        {
                            parent = block;
                        }
                        else
                        {
                            for (sl::HLSLStatement* bs = block->statement; bs; bs = bs->nextStatement)
                            {
                                if (bs->nextStatement == target)
                                {
                                    parent = block;
                                    prev = bs;
                                    break;
                                }
                            }
                        }
                    }
                };

                // actual inlining happens here :
                //  * make block-node with all statements from function-body (and remove original func.declaration)
                //  * add temp.variables inside block inited with values/expressions of func.call arguments
                //  * add 'retval' variable (outside func.block), make assignment of function ret.value to this 'retval' variable

                FindParentPrev find_parent_prev;
                find_parent_prev.target = find_statement_expression.statement;
                find_parent_prev.VisitRoot(ast->GetRoot());

                sl::HLSLStatement* statement = find_statement_expression.statement;
                sl::HLSLDeclaration* rv_decl = ast->AddNode<sl::HLSLDeclaration>(statement->fileName, statement->line);
                sl::HLSLBlockStatement* func_block = ast->AddNode<sl::HLSLBlockStatement>(statement->fileName, statement->line);
                char ret_name[128];

                Snprintf(ret_name, sizeof(ret_name), "retval_%s_%u", func_decl->name, k);

                rv_decl->type.baseType = func_decl->returnType.baseType;
                rv_decl->name = ast->AddString(ret_name);

                if (find_statement_expression.statement->nodeType == sl::HLSLNodeType_Declaration
                    && (static_cast<sl::HLSLDeclaration*>(find_statement_expression.statement))->assignment == fcall[k]
                    )
                {
                    sl::HLSLDeclaration* decl = static_cast<sl::HLSLDeclaration*>(find_statement_expression.statement);
                    sl::HLSLIdentifierExpression* val = ast->AddNode<sl::HLSLIdentifierExpression>(statement->fileName, statement->line);

                    val->name = ast->AddString(ret_name);

                    DVASSERT(decl->assignment == fcall[k]);
                    decl->assignment = val;
                }
                else
                {
                    switch (find_statement_expression.expr->nodeType)
                    {
                    case sl::HLSLNodeType_BinaryExpression:
                    {
                        sl::HLSLBinaryExpression* bin = static_cast<sl::HLSLBinaryExpression*>(find_statement_expression.expr);
                        sl::HLSLExpression** ce = NULL;

                        if (bin->expression1 == fcall[k])
                            ce = &(bin->expression1);
                        else if (bin->expression2 == fcall[k])
                            ce = &(bin->expression2);

                        DVASSERT(ce);

                        sl::HLSLIdentifierExpression* val = ast->AddNode<sl::HLSLIdentifierExpression>(statement->fileName, statement->line);

                        val->name = ast->AddString(ret_name);
                        (*ce) = val;
                    }
                    break;
                    case sl::HLSLNodeType_FunctionCall:
                    {
                        sl::HLSLFunctionCall* call = static_cast<sl::HLSLFunctionCall*>(find_statement_expression.expr);
                        sl::HLSLExpression* prev_arg = call->argument;

                        for (sl::HLSLExpression* arg = call->argument; arg; arg = arg->nextExpression)
                        {
                            if (arg == fcall[k])
                            {
                                sl::HLSLIdentifierExpression* val = ast->AddNode<sl::HLSLIdentifierExpression>(statement->fileName, statement->line);

                                val->name = ast->AddString(ret_name);
                                val->nextExpression = arg->nextExpression;
                                if (arg == call->argument)
                                    call->argument = val;
                                else
                                    prev_arg->nextExpression = val;
                                break;
                            }
                            prev_arg = arg;
                        }
                    }
                    break;
                    default:
                        break; // to shut up goddamn warning
                    }
                }

                sl::HLSLStatement* func_parent = NULL;
                sl::HLSLFunction* func = ast->FindFunction(func_decl->name, &func_parent);
                sl::HLSLStatement* func_ret_parent = NULL;
                sl::HLSLExpression* func_ret = NULL;
                DVASSERT(func);
                DVASSERT(func_parent);

                sl::HLSLStatement* sp = NULL;
                for (sl::HLSLStatement* fs = func->statement; fs; fs = fs->nextStatement)
                {
                    if (fs->nodeType == sl::HLSLNodeType_ReturnStatement)
                    {
                        func_ret = (static_cast<sl::HLSLReturnStatement*>(fs))->expression;
                        func_ret_parent = sp;
                        break;
                    }
                    sp = fs;
                }
                DVASSERT(func_ret);

                std::vector<sl::HLSLDeclaration*> arg;

                {
                    int a = 0;
                    for (sl::HLSLExpression *e = fcall[k]->argument; e; e = e->nextExpression, ++a)
                    {
                        sl::HLSLDeclaration* arg_decl = ast->AddNode<sl::HLSLDeclaration>(statement->fileName, statement->line);

                        int aa = 0;
                        for (sl::HLSLArgument *fa = func->argument; fa; fa = fa->nextArgument, ++aa)
                        {
                            if (aa == a)
                            {
                                arg_decl->name = fa->name;
                                arg_decl->type = fa->type;
                                break;
                            }
                        }

                        arg_decl->assignment = e;
                        arg.push_back(arg_decl);
                    }
                }

                sl::HLSLBinaryExpression* ret_ass = ast->AddNode<sl::HLSLBinaryExpression>(statement->fileName, statement->line);
                sl::HLSLIdentifierExpression* ret_var = ast->AddNode<sl::HLSLIdentifierExpression>(statement->fileName, statement->line);
                sl::HLSLExpressionStatement* ret_statement = ast->AddNode<sl::HLSLExpressionStatement>(statement->fileName, statement->line);

                ret_statement->expression = ret_ass;
                ret_var->name = ast->AddString(ret_name);
                ret_ass->binaryOp = sl::HLSLBinaryOp_Assign;
                ret_ass->expressionType = func_decl->returnType;
                ret_ass->expression1 = ret_var;
                ret_ass->expression2 = func_ret;

                if (func_ret_parent)
                {
                    func_ret_parent->nextStatement = ret_statement;
                }
                else
                {
                    // func has exactly one statement
                    func->statement = ret_statement;
                }

                if (find_parent_prev.parent && !find_parent_prev.prev)
                {
                    DVASSERT(find_parent_prev.parent->nodeType == sl::HLSLNodeType_BlockStatement);
                    sl::HLSLBlockStatement* block = static_cast<sl::HLSLBlockStatement*>(find_parent_prev.parent);
                    sl::HLSLStatement* next = block->statement;

                    block->statement = rv_decl;
                    rv_decl->nextStatement = func_block;
                    func_block->nextStatement = next;
                }
                else
                {
                    find_parent_prev.prev->nextStatement = rv_decl;
                    rv_decl->nextStatement = func_block;
                    func_block->nextStatement = statement;
                }

                DVASSERT(arg.size());
                func_block->statement = arg[0];
                for (int i = 1; i < int(arg.size()); ++i)
                {
                    arg[i - 1]->nextStatement = arg[i];
                }
                arg[arg.size() - 1]->nextStatement = func->statement;
            }
            else
            {
                DVASSERT(!"kaboom!");
            }
        } // for each func.call

        // remove func definition
        if (!keep_func_def)
        {
            sl::HLSLStatement* func_parent = NULL;
            sl::HLSLFunction* func = ast->FindFunction(func_decl->name, &func_parent);

            if (func && func_parent)
                func_parent->nextStatement = func->nextStatement;
        }
    } // for each func
}

//------------------------------------------------------------------------------

bool
ShaderSource::ProcessMetaData(sl::HLSLTree* ast)
{
    struct
    prop_t
    {
        sl::HLSLDeclaration* decl;
        sl::HLSLStatement* prev_statement;
    };

    std::vector<prop_t> prop_decl;
    char btype = 'x';

    if (ast->FindGlobalStruct("vertex_in"))
        btype = 'V';
    else if (ast->FindGlobalStruct("fragment_in"))
        btype = 'F';

    // find properties/samplers
    {
        sl::HLSLStatement* statement = ast->GetRoot()->statement;
        sl::HLSLStatement* pstatement = NULL;
        unsigned sampler_reg = 0;

        while (statement)
        {
            if (statement->nodeType == sl::HLSLNodeType_Declaration)
            {
                sl::HLSLDeclaration* decl = static_cast<sl::HLSLDeclaration*>(statement);

                if (decl->type.flags & sl::HLSLTypeFlag_Property)
                {
                    property.resize(property.size() + 1);
                    rhi::ShaderProp& prop = property.back();

                    prop.uid = DAVA::FastName(decl->name);
                    prop.source = rhi::ShaderProp::SOURCE_AUTO;
                    prop.precision = rhi::ShaderProp::PRECISION_NORMAL;
                    prop.arraySize = 1;
                    prop.isBigArray = false;

                    if (decl->type.array)
                    {
                        if (decl->type.arraySize->nodeType == sl::HLSLNodeType_LiteralExpression)
                        {
                            sl::HLSLLiteralExpression* expr = static_cast<sl::HLSLLiteralExpression*>(decl->type.arraySize);

                            if (expr->type == sl::HLSLBaseType_Int)
                                prop.arraySize = expr->iValue;
                        }
                    }

                    if (decl->annotation)
                    {
                        if (strstr(decl->annotation, "bigarray"))
                        {
                            prop.isBigArray = true;
                        }
                    }

                    if (prop.arraySize > 1 && !prop.isBigArray)
                    {
                        Logger::Error("property \"%s\" is an array but not marked as \"bigarray\"", prop.uid.c_str());
                        DVASSERT(!"kaboom!");
                    }

                    switch (decl->type.baseType)
                    {
                    case sl::HLSLBaseType_Float:
                        prop.type = rhi::ShaderProp::TYPE_FLOAT1;
                        break;
                    case sl::HLSLBaseType_Float2:
                        prop.type = rhi::ShaderProp::TYPE_FLOAT2;
                        break;
                    case sl::HLSLBaseType_Float3:
                        prop.type = rhi::ShaderProp::TYPE_FLOAT3;
                        break;
                    case sl::HLSLBaseType_Float4:
                        prop.type = rhi::ShaderProp::TYPE_FLOAT4;
                        break;
                    case sl::HLSLBaseType_Float4x4:
                        prop.type = rhi::ShaderProp::TYPE_FLOAT4X4;
                        break;
                    default:
                        break; // to shut up goddamn warning
                    }

                    for (sl::HLSLAttribute* a = decl->attributes; a; a = a->nextAttribute)
                    {
                        if (stricmp(a->attrText, "material") == 0)
                            prop.source = rhi::ShaderProp::SOURCE_MATERIAL;
                        else if (stricmp(a->attrText, "auto") == 0)
                            prop.source = rhi::ShaderProp::SOURCE_AUTO;
                        else
                            prop.tag = FastName(a->attrText);
                    }

                    if (decl->assignment)
                    {
                        if (decl->assignment->nodeType == sl::HLSLNodeType_ConstructorExpression)
                        {
                            sl::HLSLConstructorExpression* ctor = static_cast<sl::HLSLConstructorExpression*>(decl->assignment);
                            unsigned val_i = 0;

                            for (sl::HLSLExpression *arg = ctor->argument; arg; arg = arg->nextExpression, ++val_i)
                            {
                                if (arg->nodeType == sl::HLSLNodeType_LiteralExpression)
                                {
                                    sl::HLSLLiteralExpression* expr = static_cast<sl::HLSLLiteralExpression*>(arg);

                                    if (expr->type == sl::HLSLBaseType_Float)
                                        prop.defaultValue[val_i] = expr->fValue;
                                    else if (expr->type == sl::HLSLBaseType_Int)
                                        prop.defaultValue[val_i] = float(expr->iValue);
                                }
                            }
                        }
                        else if (decl->assignment->nodeType == sl::HLSLNodeType_LiteralExpression)
                        {
                            sl::HLSLLiteralExpression* expr = static_cast<sl::HLSLLiteralExpression*>(decl->assignment);

                            if (expr->type == sl::HLSLBaseType_Float)
                                prop.defaultValue[0] = expr->fValue;
                            else if (expr->type == sl::HLSLBaseType_Int)
                                prop.defaultValue[0] = float(expr->iValue);
                        }
                    }

                    buf_t* cbuf = nullptr;

                    for (std::vector<buf_t>::iterator b = buf.begin(), b_end = buf.end(); b != b_end; ++b)
                    {
                        if (b->source == prop.source && b->tag == prop.tag)
                        {
                            cbuf = &(buf[b - buf.begin()]);
                            break;
                        }
                    }

                    if (!cbuf)
                    {
                        buf.resize(buf.size() + 1);

                        cbuf = &(buf.back());

                        cbuf->source = prop.source;
                        cbuf->tag = prop.tag;
                        cbuf->regCount = 0;
                        cbuf->isArray = false;
                    }

                    prop.bufferindex = static_cast<uint32>(cbuf - &(buf[0]));

                    if (prop.type == rhi::ShaderProp::TYPE_FLOAT1 || prop.type == rhi::ShaderProp::TYPE_FLOAT2 || prop.type == rhi::ShaderProp::TYPE_FLOAT3)
                    {
                        bool do_add = true;
                        uint32 sz = 0;

                        switch (prop.type)
                        {
                        case rhi::ShaderProp::TYPE_FLOAT1:
                            sz = 1;
                            break;
                        case rhi::ShaderProp::TYPE_FLOAT2:
                            sz = 2;
                            break;
                        case rhi::ShaderProp::TYPE_FLOAT3:
                            sz = 3;
                            break;
                        default:
                            break;
                        }

                        for (unsigned r = 0; r != cbuf->avlRegIndex.size(); ++r)
                        {
                            if (cbuf->avlRegIndex[r] + sz <= 4)
                            {
                                prop.bufferReg = r;
                                prop.bufferRegCount = cbuf->avlRegIndex[r];

                                cbuf->avlRegIndex[r] += sz;

                                do_add = false;
                                break;
                            }
                        }

                        if (do_add)
                        {
                            prop.bufferReg = cbuf->regCount;
                            prop.bufferRegCount = 0;

                            ++cbuf->regCount;

                            cbuf->avlRegIndex.push_back(sz);
                        }
                    }
                    else if (prop.type == rhi::ShaderProp::TYPE_FLOAT4 || prop.type == rhi::ShaderProp::TYPE_FLOAT4X4)
                    {
                        prop.bufferReg = cbuf->regCount;
                        prop.bufferRegCount = prop.arraySize * ((prop.type == rhi::ShaderProp::TYPE_FLOAT4) ? 1 : 4);

                        cbuf->regCount += prop.bufferRegCount;

                        if (prop.arraySize > 1)
                        {
                            cbuf->isArray = true;
                        }
                        else
                        {
                            for (int i = 0; i != prop.bufferRegCount; ++i)
                                cbuf->avlRegIndex.push_back(4);
                        }
                    }

                    prop_t pp;

                    pp.decl = decl;
                    pp.prev_statement = pstatement;

                    prop_decl.push_back(pp);
                }

                if (decl->type.baseType == sl::HLSLBaseType_Sampler2D
                    || decl->type.baseType == sl::HLSLBaseType_SamplerCube
                    || decl->type.baseType == sl::HLSLBaseType_Sampler2DShadow
                    )
                {
                    sampler.resize(sampler.size() + 1);
                    rhi::ShaderSampler& s = sampler.back();

                    char regName[128];

                    switch (decl->type.baseType)
                    {
                    case sl::HLSLBaseType_Sampler2D:
                    case sl::HLSLBaseType_Sampler2DShadow:
                        s.type = rhi::TEXTURE_TYPE_2D;
                        break;
                    case sl::HLSLBaseType_SamplerCube:
                        s.type = rhi::TEXTURE_TYPE_CUBE;
                        break;
                    default:
                        break; // to shut up goddamn warning
                    }
                    s.uid = FastName(decl->name);
                    Snprintf(regName, sizeof(regName), "s%u", sampler_reg);
                    ++sampler_reg;
                    decl->registerName = ast->AddString(regName);
                }
            }

            pstatement = statement;
            statement = statement->nextStatement;
        }
    }

    // rename vertex-input variables to pre-defined names (needed by GL-backend)
    {
        sl::HLSLStruct* vinput = ast->FindGlobalStruct("vertex_in");
        if (vinput)
        {
            const char* vertex_in = ast->AddString("vertex_in");

            class Replacer : public sl::HLSLTreeVisitor
            {
            public:
                sl::HLSLStructField* field;
                const char* new_name;
                const char* vertex_in;
                virtual void VisitMemberAccess(sl::HLSLMemberAccess* node)
                {
                    if (node->field == field->name
                        && node->object->expressionType.baseType == sl::HLSLBaseType_UserDefined
                        && node->object->expressionType.typeName == vertex_in
                        )
                    {
                        node->field = new_name;
                    }

                    sl::HLSLTreeVisitor::VisitMemberAccess(node);
                }
                void Replace(sl::HLSLTree* ast, sl::HLSLStructField* field_, const char* new_name_)
                {
                    field = field_;
                    new_name = new_name_;
                    VisitRoot(ast->GetRoot());

                    field->name = new_name_;
                }
            };

            Replacer r;

            r.vertex_in = vertex_in;

            struct
            {
                const char* semantic;
                const char* attr_name;
            } attr[] =
            {
              { "POSITION", "position0" },
              { "POSITION0", "position0" },
              { "POSITION1", "position1" },
              { "POSITION2", "position2" },
              { "POSITION3", "position3" },
              { "NORMAL", "normal0" },
              { "NORMAL0", "normal0" },
              { "NORMAL1", "normal1" },
              { "NORMAL2", "normal2" },
              { "NORMAL3", "normal3" },
              { "TEXCOORD", "texcoord0" },
              { "TEXCOORD0", "texcoord0" },
              { "TEXCOORD1", "texcoord1" },
              { "TEXCOORD2", "texcoord2" },
              { "TEXCOORD3", "texcoord3" },
              { "TEXCOORD4", "texcoord4" },
              { "TEXCOORD5", "texcoord5" },
              { "TEXCOORD6", "texcoord6" },
              { "TEXCOORD7", "texcoord7" },
              { "COLOR", "color0" },
              { "COLOR0", "color0" },
              { "COLOR1", "color1" },
              { "TANGENT", "tangent" },
              { "BINORMAL", "binormal" },
              { "BLENDWEIGHT", "blendweight" },
              { "BLENDINDICES", "blendindex" }
            };

            for (sl::HLSLStructField* field = vinput->field; field; field = field->nextField)
            {
                if (field->semantic)
                {
                    for (unsigned a = 0; a != countof(attr); ++a)
                    {
                        if (stricmp(field->semantic, attr[a].semantic) == 0)
                        {
                            r.Replace(ast, field, ast->AddString(attr[a].attr_name));
                            break;
                        }
                    }
                }
            }
        }
    }

    // get vertex-layout
    {
        sl::HLSLStruct* input = ast->FindGlobalStruct("vertex_in");

        if (input)
        {
            struct
            {
                rhi::VertexSemantics usage;
                const char* semantic;
            }
            semantic[] =
            {
              { rhi::VS_POSITION, "POSITION" },
              { rhi::VS_NORMAL, "NORMAL" },
              { rhi::VS_COLOR, "COLOR" },
              { rhi::VS_TEXCOORD, "TEXCOORD" },
              { rhi::VS_TANGENT, "TANGENT" },
              { rhi::VS_BINORMAL, "BINORMAL" },
              { rhi::VS_BLENDWEIGHT, "BLENDWEIGHT" },
              { rhi::VS_BLENDINDEX, "BLENDINDICES" }
            };

            vertexLayout.Clear();

            rhi::VertexDataFrequency cur_freq = rhi::VDF_PER_VERTEX;

            for (sl::HLSLStructField* field = input->field; field; field = field->nextField)
            {
                rhi::VertexSemantics usage = rhi::VS_MAXCOUNT;
                unsigned usage_i = 0;
                rhi::VertexDataType data_type = rhi::VDT_FLOAT;
                unsigned data_count = 0;
                rhi::VertexDataFrequency freq = rhi::VDF_PER_VERTEX;

                switch (field->type.baseType)
                {
                case sl::HLSLBaseType_Float4:
                    data_type = rhi::VDT_FLOAT;
                    data_count = 4;
                    break;
                case sl::HLSLBaseType_Float3:
                    data_type = rhi::VDT_FLOAT;
                    data_count = 3;
                    break;
                case sl::HLSLBaseType_Float2:
                    data_type = rhi::VDT_FLOAT;
                    data_count = 2;
                    break;
                case sl::HLSLBaseType_Float:
                    data_type = rhi::VDT_FLOAT;
                    data_count = 1;
                    break;
                case sl::HLSLBaseType_Uint4:
                    data_type = rhi::VDT_UINT8;
                    data_count = 4;
                    break;
                default:
                    break; // to shut up goddamn warning
                }

                char sem[128];

                strcpy(sem, field->semantic);
                for (char* s = sem; *s; ++s)
                    *s = toupper(*s);

                for (unsigned i = 0; i != countof(semantic); ++i)
                {
                    const char* t = strstr(sem, semantic[i].semantic);

                    if (t == sem)
                    {
                        const char* tu = sem + strlen(semantic[i].semantic);

                        usage = semantic[i].usage;
                        usage_i = atoi(tu);

                        break;
                    }
                }

                if (usage == rhi::VS_COLOR)
                {
                    data_type = rhi::VDT_UINT8N;
                    data_count = 4;
                }

                if (field->attribute)
                {
                    if (stricmp(field->attribute->attrText, "vertex") == 0)
                        freq = rhi::VDF_PER_VERTEX;
                    else if (stricmp(field->attribute->attrText, "instance") == 0)
                        freq = rhi::VDF_PER_INSTANCE;
                }

                if (freq != cur_freq)
                    vertexLayout.AddStream(freq);
                cur_freq = freq;

                vertexLayout.AddElement(usage, usage_i, data_type, data_count);
            }
        }
    }

    if (prop_decl.size())
    {
        std::vector<sl::HLSLBuffer*> cbuf_decl;

        cbuf_decl.resize(buf.size());
        for (unsigned i = 0; i != buf.size(); ++i)
        {
            sl::HLSLBuffer* cbuf = ast->AddNode<sl::HLSLBuffer>(prop_decl[0].decl->fileName, prop_decl[0].decl->line);
            sl::HLSLDeclaration* decl = ast->AddNode<sl::HLSLDeclaration>(prop_decl[0].decl->fileName, prop_decl[0].decl->line);
            sl::HLSLLiteralExpression* sz = ast->AddNode<sl::HLSLLiteralExpression>(prop_decl[0].decl->fileName, prop_decl[0].decl->line);
            ;
            char buf_name[128];
            char buf_type_name[128];
            char buf_reg_name[128];

            Snprintf(buf_name, sizeof(buf_name), "%cP_Buffer%u", btype, i);
            Snprintf(buf_type_name, sizeof(buf_name), "%cP_Buffer%u_t", btype, i);
            Snprintf(buf_reg_name, sizeof(buf_name), "b%u", i);

            decl->name = ast->AddString(buf_name);
            decl->type.baseType = sl::HLSLBaseType_Float4;
            decl->type.array = true;
            decl->type.arraySize = sz;

            if (buf[i].isArray)
            {
                unsigned propCount = 0;

                for (unsigned p = 0; p != property.size(); ++p)
                {
                    if (property[p].bufferindex == i)
                        ++propCount;
                }
                if (propCount != 1)
                {
                    Logger::Error("cbuffer with array-property has more than one property :");
                    Logger::Error("  cbuf[%u]", i);
                    for (unsigned p = 0; p != property.size(); ++p)
                    {
                        if (property[p].bufferindex == i)
                            Logger::Error("    %s", property[p].uid.c_str());
                    }
                    return false;
                }

                for (unsigned p = 0; p != property.size(); ++p)
                {
                    if (property[p].bufferindex == i)
                    {
                        decl->name = ast->AddString(property[p].uid.c_str());
                        break;
                    }
                }

                decl->annotation = ast->AddString("bigarray");
                decl->registerName = ast->AddString(buf_name);
            }

            sz->type = sl::HLSLBaseType_Int;
            sz->iValue = buf[i].regCount;

            cbuf->field = decl;
            cbuf->name = ast->AddString(buf_type_name);
            cbuf->registerName = ast->AddString(buf_reg_name);
            cbuf->registerCount = buf[i].regCount;

            cbuf_decl[i] = cbuf;
        }

        for (unsigned i = 0; i != cbuf_decl.size() - 1; ++i)
            cbuf_decl[i]->nextStatement = cbuf_decl[i + 1];

        prop_decl[0].prev_statement->nextStatement = cbuf_decl[0];
        cbuf_decl[cbuf_decl.size() - 1]->nextStatement = prop_decl[0].decl;

        #define DO_FLOAT4_CAST 1

        DVASSERT(property.size() == prop_decl.size());
        for (unsigned i = 0; i != property.size(); ++i)
        {
            switch (property[i].type)
            {
            case rhi::ShaderProp::TYPE_FLOAT4:
            {
                if (!property[i].isBigArray)
                {
                    sl::HLSLArrayAccess* arr_access = ast->AddNode<sl::HLSLArrayAccess>(prop_decl[i].decl->fileName, prop_decl[i].decl->line);
                    sl::HLSLLiteralExpression* idx = ast->AddNode<sl::HLSLLiteralExpression>(prop_decl[0].decl->fileName, prop_decl[0].decl->line);
                    sl::HLSLIdentifierExpression* arr = ast->AddNode<sl::HLSLIdentifierExpression>(prop_decl[0].decl->fileName, prop_decl[0].decl->line);
                    char buf_name[128];

                    Snprintf(buf_name, sizeof(buf_name), "%cP_Buffer%u", btype, property[i].bufferindex);
                    arr->name = ast->AddString(buf_name);
                    arr->global = true;

                    idx->type = sl::HLSLBaseType_Int;
                    idx->iValue = property[i].bufferReg;

                    arr_access->array = arr;
                    arr_access->index = idx;
                    
                    #if DO_FLOAT4_CAST
                    sl::HLSLCastingExpression* cast_expr = ast->AddNode<sl::HLSLCastingExpression>(prop_decl[i].decl->fileName, prop_decl[i].decl->line);
                    cast_expr->expression = arr_access;
                    cast_expr->type.baseType = sl::HLSLBaseType_Float4;

                    prop_decl[i].decl->assignment = cast_expr;
                    prop_decl[i].decl->type.flags |= sl::HLSLTypeFlag_Static | sl::HLSLTypeFlag_Property;
                    #else
                    prop_decl[i].decl->assignment = arr_access;
                    prop_decl[i].decl->type.flags |= sl::HLSLTypeFlag_Static | sl::HLSLTypeFlag_Property;
                    #endif
                }
                else
                {
                    char buf_name[128];

                    Snprintf(buf_name, sizeof(buf_name), "%cP_Buffer%u", btype, property[i].bufferindex);
                    prop_decl[i].decl->registerName = ast->AddString(buf_name);
                }
            }
            break;

            case rhi::ShaderProp::TYPE_FLOAT3:
            case rhi::ShaderProp::TYPE_FLOAT2:
            case rhi::ShaderProp::TYPE_FLOAT1:
            {
                sl::HLSLMemberAccess* member_access = ast->AddNode<sl::HLSLMemberAccess>(prop_decl[i].decl->fileName, prop_decl[i].decl->line);
                char xyzw[] = { 'x', 'y', 'z', 'w', '\0' };
                unsigned elem_cnt = 0;
                sl::HLSLArrayAccess* arr_access = ast->AddNode<sl::HLSLArrayAccess>(prop_decl[i].decl->fileName, prop_decl[i].decl->line);
                sl::HLSLLiteralExpression* idx = ast->AddNode<sl::HLSLLiteralExpression>(prop_decl[0].decl->fileName, prop_decl[0].decl->line);
                sl::HLSLIdentifierExpression* arr = ast->AddNode<sl::HLSLIdentifierExpression>(prop_decl[0].decl->fileName, prop_decl[0].decl->line);
                char buf_name[128];

                switch (property[i].type)
                {
                case rhi::ShaderProp::TYPE_FLOAT1:
                    elem_cnt = 1;
                    break;
                case rhi::ShaderProp::TYPE_FLOAT2:
                    elem_cnt = 2;
                    break;
                case rhi::ShaderProp::TYPE_FLOAT3:
                    elem_cnt = 3;
                    break;
                default:
                    break; // to shut up goddamn warning
                }

                member_access->object = arr_access;
                xyzw[property[i].bufferRegCount + elem_cnt] = 0;
                member_access->field = ast->AddString(xyzw + property[i].bufferRegCount);

                Snprintf(buf_name, sizeof(buf_name), "%cP_Buffer%u", btype, property[i].bufferindex);
                arr->name = ast->AddString(buf_name);
                arr->global = true;

                idx->type = sl::HLSLBaseType_Int;
                idx->iValue = property[i].bufferReg;

                arr_access->array = arr;
                arr_access->index = idx;

                #if DO_FLOAT4_CAST
                sl::HLSLCastingExpression* cast_expr = ast->AddNode<sl::HLSLCastingExpression>(prop_decl[i].decl->fileName, prop_decl[i].decl->line);
                cast_expr->expression = arr_access;
                cast_expr->type.baseType = sl::HLSLBaseType_Float4;
                member_access->object = cast_expr;

                prop_decl[i].decl->assignment = member_access;
                prop_decl[i].decl->type.flags |= sl::HLSLTypeFlag_Static | sl::HLSLTypeFlag_Property;
                #else
                prop_decl[i].decl->assignment = member_access;
                prop_decl[i].decl->type.flags |= sl::HLSLTypeFlag_Static | sl::HLSLTypeFlag_Property;
                #endif
            }
            break;

            case rhi::ShaderProp::TYPE_FLOAT4X4:
            {
                sl::HLSLConstructorExpression* ctor = ast->AddNode<sl::HLSLConstructorExpression>(prop_decl[i].decl->fileName, prop_decl[i].decl->line);
                sl::HLSLArrayAccess* arr_access[4];
                sl::HLSLCastingExpression* cast_expr[4];

                ctor->type.baseType = sl::HLSLBaseType_Float4x4;

                for (unsigned k = 0; k != 4; ++k)
                {
                    arr_access[k] = ast->AddNode<sl::HLSLArrayAccess>(prop_decl[i].decl->fileName, prop_decl[i].decl->line);

                    sl::HLSLLiteralExpression* idx = ast->AddNode<sl::HLSLLiteralExpression>(prop_decl[0].decl->fileName, prop_decl[0].decl->line);
                    sl::HLSLIdentifierExpression* arr = ast->AddNode<sl::HLSLIdentifierExpression>(prop_decl[0].decl->fileName, prop_decl[0].decl->line);
                    char buf_name[128];

                    Snprintf(buf_name, sizeof(buf_name), "%cP_Buffer%u", btype, property[i].bufferindex);
                    arr->name = ast->AddString(buf_name);

                    idx->type = sl::HLSLBaseType_Int;
                    idx->iValue = property[i].bufferReg + k;

                    arr_access[k]->array = arr;
                    arr_access[k]->index = idx;
                }

                #if DO_FLOAT4_CAST
                for (unsigned k = 0; k != 4; ++k)
                {
                    cast_expr[k] = ast->AddNode<sl::HLSLCastingExpression>(prop_decl[i].decl->fileName, prop_decl[i].decl->line);
                    cast_expr[k]->expression = arr_access[k];
                    cast_expr[k]->type.baseType = sl::HLSLBaseType_Float4;
                }

                ctor->argument = cast_expr[0];
                for (unsigned k = 0; k != 4 - 1; ++k)
                    cast_expr[k]->nextExpression = cast_expr[k + 1];
                #else
                ctor->argument = arr_access[0];
                for (unsigned k = 0; k != 4 - 1; ++k)
                    arr_access[k]->nextExpression = arr_access[k + 1];
                #endif

                prop_decl[i].decl->assignment = ctor;
                prop_decl[i].decl->type.flags |= sl::HLSLTypeFlag_Static | sl::HLSLTypeFlag_Property;
            }
            break;
            default:
                break; // to shut up goddamn warning
            }

            if (property[i].isBigArray)
            {
                prop_decl[i].decl->hidden = true;
            }
        }
    }

    // get blending

    for (int i = 0; i != MAX_RENDER_TARGET_COUNT; ++i)
    {
        sl::HLSLBlend* blend = ast->GetRoot()->blend[i];

        if (blend)
        {
            blending.rtBlend[i].blendEnabled = !(blend->src_op == sl::BLENDOP_ONE && blend->dst_op == sl::BLENDOP_ZERO);

            switch (blend->src_op)
            {
            case sl::BLENDOP_ZERO:
                blending.rtBlend[i].colorSrc = rhi::BLENDOP_ZERO;
                break;
            case sl::BLENDOP_ONE:
                blending.rtBlend[i].colorSrc = rhi::BLENDOP_ONE;
                break;
            case sl::BLENDOP_SRC_ALPHA:
                blending.rtBlend[i].colorSrc = rhi::BLENDOP_SRC_ALPHA;
                break;
            case sl::BLENDOP_INV_SRC_ALPHA:
                blending.rtBlend[i].colorSrc = rhi::BLENDOP_INV_SRC_ALPHA;
                break;
            case sl::BLENDOP_SRC_COLOR:
                blending.rtBlend[i].colorSrc = rhi::BLENDOP_SRC_COLOR;
                break;
            case sl::BLENDOP_DST_COLOR:
                blending.rtBlend[i].colorSrc = rhi::BLENDOP_DST_COLOR;
                break;
            }
            switch (blend->dst_op)
            {
            case sl::BLENDOP_ZERO:
                blending.rtBlend[i].colorDst = rhi::BLENDOP_ZERO;
                break;
            case sl::BLENDOP_ONE:
                blending.rtBlend[i].colorDst = rhi::BLENDOP_ONE;
                break;
            case sl::BLENDOP_SRC_ALPHA:
                blending.rtBlend[i].colorDst = rhi::BLENDOP_SRC_ALPHA;
                break;
            case sl::BLENDOP_INV_SRC_ALPHA:
                blending.rtBlend[i].colorDst = rhi::BLENDOP_INV_SRC_ALPHA;
                break;
            case sl::BLENDOP_SRC_COLOR:
                blending.rtBlend[i].colorDst = rhi::BLENDOP_SRC_COLOR;
                break;
            case sl::BLENDOP_DST_COLOR:
                blending.rtBlend[i].colorDst = rhi::BLENDOP_DST_COLOR;
                break;
            }
        }
    }

    // get color write-mask

    for (int i = 0; i != MAX_RENDER_TARGET_COUNT; ++i)
    {
        sl::HLSLColorMask* mask = ast->GetRoot()->color_mask[i];

        if (mask)
        {
            switch (mask->mask)
            {
            case sl::COLORMASK_NONE:
                blending.rtBlend[i].writeMask = rhi::COLORMASK_NONE;
                break;
            case sl::COLORMASK_ALL:
                blending.rtBlend[i].writeMask = rhi::COLORMASK_ALL;
                break;
            case sl::COLORMASK_RGB:
                blending.rtBlend[i].writeMask = rhi::COLORMASK_R | rhi::COLORMASK_G | rhi::COLORMASK_B;
                break;
            case sl::COLORMASK_A:
                blending.rtBlend[i].writeMask = rhi::COLORMASK_A;
                break;
            }
        }
    }

#if 0
    Logger::Info("properties (%u) :", property.size());
    for (std::vector<rhi::ShaderProp>::const_iterator p = property.begin(), p_end = property.end(); p != p_end; ++p)
    {
        if (p->type == rhi::ShaderProp::TYPE_FLOAT4 || p->type == rhi::ShaderProp::TYPE_FLOAT4X4)
        {
            if (p->arraySize == 1)
            {
                Logger::Info("  %-16s    buf#%u  -  %u, %u x float4", p->uid.c_str(), p->bufferindex, p->bufferReg, p->bufferRegCount);
            }
            else
            {
                char name[128];

                Snprintf(name, sizeof(name) - 1, "%s[%u]", p->uid.c_str(), p->arraySize);
                Logger::Info("  %-16s    buf#%u  -  %u, %u x float4", name, p->bufferindex, p->bufferReg, p->bufferRegCount);
            }
        }
        else
        {
            const char* xyzw = "xyzw";

            switch (p->type)
            {
            case rhi::ShaderProp::TYPE_FLOAT1:
                Logger::Info("  %-16s    buf#%u  -  %u, %c", p->uid.c_str(), p->bufferindex, p->bufferReg, xyzw[p->bufferRegCount]);
                break;

            case rhi::ShaderProp::TYPE_FLOAT2:
                Logger::Info("  %-16s    buf#%u  -  %u, %c%c", p->uid.c_str(), p->bufferindex, p->bufferReg, xyzw[p->bufferRegCount + 0], xyzw[p->bufferRegCount + 1]);
                break;

            case rhi::ShaderProp::TYPE_FLOAT3:
                Logger::Info("  %-16s    buf#%u  -  %u, %c%c%c", p->uid.c_str(), p->bufferindex, p->bufferReg, xyzw[p->bufferRegCount + 0], xyzw[p->bufferRegCount + 1], xyzw[p->bufferRegCount + 2]);
                break;

            default:
                break;
            }
        }
    }

    Logger::Info("\n--- ShaderSource");
    Logger::Info("buffers (%u) :", buf.size());
    for (unsigned i = 0; i != buf.size(); ++i)
    {
        Logger::Info("  buf#%u  reg.count = %u", i, buf[i].regCount);
    }

    Logger::Info("samplers (%u) :", sampler.size());
    for (unsigned s = 0; s != sampler.size(); ++s)
    {
        Logger::Info("  sampler#%u  \"%s\"", s, sampler[s].uid.c_str());
    }
    Logger::Info("\n\n");
#endif

    return true;
};

//------------------------------------------------------------------------------

static inline bool
ReadUI1(DAVA::File* f, uint8* x)
{
    return (f->Read(x) == sizeof(uint8));
}

static inline bool
ReadUI4(DAVA::File* f, uint32* x)
{
    return (f->Read(x) == sizeof(uint32));
}

static inline bool
ReadS0(DAVA::File* f, std::string* str)
{
    char s0[128 * 1024];
    uint32 sz = 0;
    if (ReadUI4(f, &sz))
    {
        if (f->Read(s0, sz) == sz)
        {
            *str = s0;
            return true;
        }
    }
    return false;
}

bool ShaderSource::Load(Api api, DAVA::File* in)
{
#define READ_CHECK(exp) if (!(exp)) { return false; }

    std::string s0;
    uint32 readUI4;
    uint8 readUI1;

    Reset();

    READ_CHECK(ReadUI4(in, &readUI4));
    type = ProgType(readUI4);

    READ_CHECK(ReadS0(in, code + api));

    READ_CHECK(vertexLayout.Load(in));

    READ_CHECK(ReadUI4(in, &readUI4));
    READ_CHECK(readUI4 <= rhi::MAX_SHADER_PROPERTY_COUNT);
    property.resize(readUI4);
    for (unsigned p = 0; p != property.size(); ++p)
    {
        READ_CHECK(ReadS0(in, &s0));
        property[p].uid = FastName(s0.c_str());

        READ_CHECK(ReadS0(in, &s0));
        property[p].tag = FastName(s0.c_str());

        READ_CHECK(ReadUI4(in, &readUI4));
        property[p].type = ShaderProp::Type(readUI4);

        READ_CHECK(ReadUI4(in, &readUI4));
        property[p].source = ShaderProp::Source(readUI4);

        READ_CHECK(ReadUI4(in, &readUI4));
        property[p].isBigArray = readUI4;

        READ_CHECK(ReadUI4(in, &property[p].arraySize));
        READ_CHECK(ReadUI4(in, &property[p].bufferindex));
        READ_CHECK(ReadUI4(in, &property[p].bufferReg));
        READ_CHECK(ReadUI4(in, &property[p].bufferRegCount));

        READ_CHECK(in->Read(property[p].defaultValue, 16 * sizeof(float)) == 16 * sizeof(float));
    }

    READ_CHECK(ReadUI4(in, &readUI4));
    READ_CHECK(readUI4 <= rhi::MAX_SHADER_CONST_BUFFER_COUNT);
    uint32 bufCount = readUI4;
    for (const ShaderProp& p : property)
        READ_CHECK(p.bufferindex < bufCount);

    buf.resize(bufCount);
    for (unsigned b = 0; b != buf.size(); ++b)
    {
        READ_CHECK(ReadUI4(in, &readUI4));
        buf[b].source = ShaderProp::Source(readUI4);

        READ_CHECK(ReadS0(in, &s0));
        buf[b].tag = FastName(s0.c_str());

        READ_CHECK(ReadUI4(in, &buf[b].regCount));
    }

    READ_CHECK(ReadUI4(in, &readUI4));
    READ_CHECK(readUI4 <= (rhi::MAX_FRAGMENT_TEXTURE_SAMPLER_COUNT + rhi::MAX_VERTEX_TEXTURE_SAMPLER_COUNT))
    sampler.resize(readUI4);
    for (unsigned s = 0; s != sampler.size(); ++s)
    {
        READ_CHECK(ReadUI4(in, &readUI4));
        sampler[s].type = TextureType(readUI4);

        READ_CHECK(ReadS0(in, &s0));
        sampler[s].uid = FastName(s0.c_str());
    }

    for (int i = 0; i != MAX_RENDER_TARGET_COUNT; ++i)
    {
        READ_CHECK(ReadUI1(in, &readUI1));
        blending.rtBlend[i].colorFunc = readUI1;
        READ_CHECK(ReadUI1(in, &readUI1));
        blending.rtBlend[i].colorSrc = readUI1;
        READ_CHECK(ReadUI1(in, &readUI1));
        blending.rtBlend[i].colorDst = readUI1;
        READ_CHECK(ReadUI1(in, &readUI1));
        blending.rtBlend[i].alphaFunc = readUI1;
        READ_CHECK(ReadUI1(in, &readUI1));
        blending.rtBlend[i].alphaSrc = readUI1;
        READ_CHECK(ReadUI1(in, &readUI1));
        blending.rtBlend[i].alphaDst = readUI1;
        READ_CHECK(ReadUI1(in, &readUI1));
        blending.rtBlend[i].writeMask = readUI1;
        READ_CHECK(ReadUI1(in, &readUI1));
        blending.rtBlend[i].blendEnabled = readUI1;
        READ_CHECK(ReadUI1(in, &readUI1));
        blending.rtBlend[i].alphaToCoverage = readUI1;
        READ_CHECK(in->Seek(3, DAVA::File::SEEK_FROM_CURRENT));
    }
    
#undef READ_CHECK

    return true;
}

//------------------------------------------------------------------------------

static inline bool
WriteUI1(DAVA::File* f, uint8 x)
{
    return (f->Write(&x) == sizeof(x));
}

static inline bool
WriteUI4(DAVA::File* f, uint32 x)
{
    return (f->Write(&x) == sizeof(x));
}

static inline bool
WriteS0(DAVA::File* f, const char* str)
{
    char s0[128 * 1024];
    uint32 sz = L_ALIGNED_SIZE(strlen(str) + 1, sizeof(uint32));

    memset(s0, 0x00, sz);
    strcpy(s0, str);

    if (WriteUI4(f, sz))
    {
        return (f->Write(s0, sz) == sz);
    }
    return false;
}

bool ShaderSource::Save(Api api, DAVA::File* out) const
{
#define WRITE_CHECK(exp) if (!(exp)) { return false; }

    if (code[api].length() == 0)
    {
        if (ast)
            GetSourceCode(api);
        else
            return false;
    }

    WRITE_CHECK(WriteUI4(out, type));
    WRITE_CHECK(WriteS0(out, code[api].c_str()));

    WRITE_CHECK(vertexLayout.Save(out));

    WRITE_CHECK(WriteUI4(out, static_cast<uint32>(property.size())));
    for (unsigned p = 0; p != property.size(); ++p)
    {
        WRITE_CHECK(WriteS0(out, property[p].uid.c_str()));
        WRITE_CHECK(WriteS0(out, property[p].tag.c_str()));
        WRITE_CHECK(WriteUI4(out, property[p].type));
        WRITE_CHECK(WriteUI4(out, property[p].source));
        WRITE_CHECK(WriteUI4(out, property[p].isBigArray));
        WRITE_CHECK(WriteUI4(out, property[p].arraySize));
        WRITE_CHECK(WriteUI4(out, property[p].bufferindex));
        WRITE_CHECK(WriteUI4(out, property[p].bufferReg));
        WRITE_CHECK(WriteUI4(out, property[p].bufferRegCount));
        WRITE_CHECK(out->Write(property[p].defaultValue, 16 * sizeof(float)) == 16 * sizeof(float));
    }

    WRITE_CHECK(WriteUI4(out, static_cast<uint32>(buf.size())));
    for (unsigned b = 0; b != buf.size(); ++b)
    {
        WRITE_CHECK(WriteUI4(out, buf[b].source));
        WRITE_CHECK(WriteS0(out, buf[b].tag.c_str()));
        WRITE_CHECK(WriteUI4(out, buf[b].regCount));
    }

    WRITE_CHECK(WriteUI4(out, static_cast<uint32>(sampler.size())));
    for (unsigned s = 0; s != sampler.size(); ++s)
    {
        WRITE_CHECK(WriteUI4(out, sampler[s].type));
        WRITE_CHECK(WriteS0(out, sampler[s].uid.c_str()));
    }

    for (int i = 0; i != MAX_RENDER_TARGET_COUNT; ++i)
    {
        WRITE_CHECK(WriteUI1(out, blending.rtBlend[i].colorFunc));
        WRITE_CHECK(WriteUI1(out, blending.rtBlend[i].colorSrc));
        WRITE_CHECK(WriteUI1(out, blending.rtBlend[i].colorDst));
        WRITE_CHECK(WriteUI1(out, blending.rtBlend[i].alphaFunc));
        WRITE_CHECK(WriteUI1(out, blending.rtBlend[i].alphaSrc));
        WRITE_CHECK(WriteUI1(out, blending.rtBlend[i].alphaDst));
        WRITE_CHECK(WriteUI1(out, blending.rtBlend[i].writeMask));
        WRITE_CHECK(WriteUI1(out, blending.rtBlend[i].blendEnabled));
        WRITE_CHECK(WriteUI1(out, blending.rtBlend[i].alphaToCoverage));
        WRITE_CHECK(WriteUI1(out, 0));
        WRITE_CHECK(WriteUI1(out, 0));
        WRITE_CHECK(WriteUI1(out, 0));
    }

#undef WRITE_CHECK

    return true;
}

//------------------------------------------------------------------------------

const DAVA::String& ShaderSource::GetSourceCode(Api targetApi) const
{
    DVASSERT(targetApi < countof(code));

    if (code[targetApi].empty() && (ast != nullptr))
    {
        static sl::Allocator alloc;
        static sl::HLSLGenerator hlsl_gen(&alloc);
        static sl::GLESGenerator gles_gen(&alloc);
        static sl::MSLGenerator mtl_gen(&alloc);

        bool codeGenerated = false;
        const char* main = (type == PROG_VERTEX) ? "vp_main" : "fp_main";
        sl::Target target = (type == PROG_VERTEX) ? sl::TARGET_VERTEX : sl::TARGET_FRAGMENT;
        DAVA::String* src = code + targetApi;

        switch (targetApi)
        {
        case RHI_DX11:
            codeGenerated = hlsl_gen.Generate(ast, sl::HLSLGenerator::MODE_DX11, target, main, src);
            break;

        case RHI_DX9:
            codeGenerated = hlsl_gen.Generate(ast, sl::HLSLGenerator::MODE_DX9, target, main, src);
            break;

        case RHI_GLES2:
            codeGenerated = gles_gen.Generate(ast, target, main, src);
            break;

        case RHI_METAL:
            codeGenerated = mtl_gen.Generate(ast, target, main, src);
            break;

        case RHI_NULL_RENDERER:
            codeGenerated = true;
            *src = "NullRender shader source";
            break;

        default:
            break;
        }

        if (!codeGenerated)
            src->clear();
    }

#if 0
{
    Logger::Info("generated src-code (api=%i) :",int(targetApi));

    char ss[64 * 1024];
    unsigned line_cnt = 0;

    if (strlen(src->c_str()) < sizeof(ss))
    {
        strcpy(ss, src->c_str());

        const char* line = ss;
        for (char* s = ss; *s; ++s)
        {
            if( *s=='\r')
                *s=' ';

            if (*s == '\n')
            {
                *s = 0;
                Logger::Info("%4u |  %s", 1 + line_cnt, line);
                line = s+1;
                ++line_cnt;
            }
        }
    }
    else
    {
        Logger::Info(code->c_str());
    }
}
#endif

    return code[targetApi];
}

//------------------------------------------------------------------------------

const ShaderPropList&
ShaderSource::Properties() const
{
    return property;
}

//------------------------------------------------------------------------------

const ShaderSamplerList&
ShaderSource::Samplers() const
{
    return sampler;
}

//------------------------------------------------------------------------------

const VertexLayout&
ShaderSource::ShaderVertexLayout() const
{
    return vertexLayout;
}

//------------------------------------------------------------------------------

uint32
ShaderSource::ConstBufferCount() const
{
    return static_cast<uint32>(buf.size());
}

//------------------------------------------------------------------------------

uint32
ShaderSource::ConstBufferSize(uint32 bufIndex) const
{
    return buf[bufIndex].regCount;
}

//------------------------------------------------------------------------------

ShaderProp::Source
ShaderSource::ConstBufferSource(uint32 bufIndex) const
{
    return buf[bufIndex].source;
}

//------------------------------------------------------------------------------

BlendState
ShaderSource::Blending() const
{
    return blending;
}

//------------------------------------------------------------------------------

void ShaderSource::Reset()
{
    vertexLayout.Clear();
    property.clear();
    sampler.clear();
    buf.clear();
    //    code.clear();
    codeLineCount = 0;

    for (unsigned i = 0; i != countof(blending.rtBlend); ++i)
    {
        blending.rtBlend[i].blendEnabled = false;
        blending.rtBlend[i].alphaToCoverage = false;
    }

    for (unsigned i = 0; i != countof(code); ++i)
        code[i].clear();
}

void ShaderSource::AddIncludeDirectory(const char* dir)
{
    ShaderSourceFileCallback.AddIncludeDirectory(dir);
}

void ShaderSource::PurgeIncludesCache()
{
    ShaderSourceFileCallback.ClearCache();
}

//------------------------------------------------------------------------------

void ShaderSource::Dump() const
{
    /*
    Logger::Info("src-code:");

    char src[64 * 1024];
    char* src_line[1024];
    unsigned line_cnt = 0;

    if (strlen(code.c_str()) < sizeof(src))
    {
        strcpy(src, code.c_str());
        memset(src_line, 0, sizeof(src_line));

        src_line[line_cnt++] = src;
        for (char* s = src; *s; ++s)
        {
            if (*s == '\n' || *s == '\r')
            {
                while (*s && (*s == '\n' || *s == '\r'))
                {
                    *s = 0;
                    ++s;
                }

                if (!(*s))
                    break;

                src_line[line_cnt] = s;
                ++line_cnt;
            }
        }

        for (unsigned i = 0; i != line_cnt; ++i)
        {
            Logger::Info("%4u |  %s", 1 + i, src_line[i]);
        }
    }
    else
    {
        Logger::Info(code.c_str());
    }
*/
    Logger::Info("properties (%u) :", property.size());
    for (std::vector<ShaderProp>::const_iterator p = property.begin(), p_end = property.end(); p != p_end; ++p)
    {
        if (p->type == ShaderProp::TYPE_FLOAT4 || p->type == ShaderProp::TYPE_FLOAT4X4)
        {
            if (p->arraySize == 1)
            {
                Logger::Info("  %-16s    buf#%u  -  %u, %u x float4", p->uid.c_str(), p->bufferindex, p->bufferReg, p->bufferRegCount);
            }
            else
            {
                char name[128];

                Snprintf(name, sizeof(name) - 1, "%s[%u]", p->uid.c_str(), p->arraySize);
                Logger::Info("  %-16s    buf#%u  -  %u, %u x float4", name, p->bufferindex, p->bufferReg, p->bufferRegCount);
            }
        }
        else
        {
            const char* xyzw = "xyzw";

            switch (p->type)
            {
            case ShaderProp::TYPE_FLOAT1:
                Logger::Info("  %-16s    buf#%u  -  %u, %c", p->uid.c_str(), p->bufferindex, p->bufferReg, xyzw[p->bufferRegCount]);
                break;

            case ShaderProp::TYPE_FLOAT2:
                Logger::Info("  %-16s    buf#%u  -  %u, %c%c", p->uid.c_str(), p->bufferindex, p->bufferReg, xyzw[p->bufferRegCount + 0], xyzw[p->bufferRegCount + 1]);
                break;

            case ShaderProp::TYPE_FLOAT3:
                Logger::Info("  %-16s    buf#%u  -  %u, %c%c%c", p->uid.c_str(), p->bufferindex, p->bufferReg, xyzw[p->bufferRegCount + 0], xyzw[p->bufferRegCount + 1], xyzw[p->bufferRegCount + 2]);
                break;

            default:
                break;
            }
        }
    }

    Logger::Info("buffers (%u) :", buf.size());
    for (unsigned i = 0; i != buf.size(); ++i)
    {
        Logger::Info("  buf#%u  reg.count = %u", i, buf[i].regCount);
    }

    if (type == PROG_VERTEX)
    {
        Logger::Info("vertex-layout:");
        vertexLayout.Dump();
    }

    Logger::Info("samplers (%u) :", sampler.size());
    for (unsigned s = 0; s != sampler.size(); ++s)
    {
        Logger::Info("  sampler#%u  \"%s\"", s, sampler[s].uid.c_str());
    }
}

//==============================================================================

//version increment history:
//5 is for new shader language
//6 is after fixing Add/Update problem
//7 is after MCPP replaced with in-house pre-processor
//8 blend-state
const uint32 ShaderSourceCache::FormatVersion = 8;

Mutex shaderSourceEntryMutex;
std::vector<ShaderSourceCache::entry_t> ShaderSourceCache::Entry;

const ShaderSource* ShaderSourceCache::Get(FastName uid, uint32 srcHash)
{
    LockGuard<Mutex> guard(shaderSourceEntryMutex);

    //    Logger::Info("get-shader-src (host-api = %i)",HostApi());
    //    Logger::Info("  uid= \"%s\"",uid.c_str());
    const ShaderSource* src = nullptr;
    Api api = HostApi();

    for (std::vector<entry_t>::const_iterator e = Entry.begin(), e_end = Entry.end(); e != e_end; ++e)
    {
        if (e->uid == uid && e->api == api && e->srcHash == srcHash)
        {
            src = e->src;
            break;
        }
    }
    //    Logger::Info("  %s",(src)?"found":"not found");

    return src;
}

//------------------------------------------------------------------------------
const ShaderSource* ShaderSourceCache::Add(const char* filename, FastName uid, ProgType progType, const char* srcText, const std::vector<std::string>& defines)
{
    ShaderSource* src = new ShaderSource(filename);

    if (src->Construct(progType, srcText, defines))
    {
        LockGuard<Mutex> guard(shaderSourceEntryMutex);

        uint32 api = HostApi();
        uint32 srcHash = DAVA::HashValue_N(srcText, unsigned(strlen(srcText)));

        bool doAdd = true;
        for (std::vector<entry_t>::iterator e = Entry.begin(), e_end = Entry.end(); e != e_end; ++e)
        {
            if ((e->uid == uid) && (e->api == api))
            {
                DAVA::SafeDelete(e->src);
                e->src = src;
                e->srcHash = srcHash;
                doAdd = false;
                break;
            }
        }

        if (doAdd)
        {
            entry_t e;
            e.uid = uid;
            e.api = api;
            e.srcHash = srcHash;
            e.src = src;

            Entry.push_back(e);
        }
    }
    else
    {
        delete src;
        src = nullptr;
    }

    return src;
}

//------------------------------------------------------------------------------

void ShaderSourceCache::Clear()
{
    LockGuard<Mutex> guard(shaderSourceEntryMutex);

    for (std::vector<entry_t>::const_iterator e = Entry.begin(), e_end = Entry.end(); e != e_end; ++e)
        delete e->src;
    Entry.clear();
}

//------------------------------------------------------------------------------

void ShaderSourceCache::Save(const char* fileName)
{
    using namespace DAVA;

    static const FilePath cacheTempFile("~doc:/shader_source_cache_temp.bin");

    File* file = File::Create(cacheTempFile, File::WRITE | File::CREATE);
    if (file)
    {
        Logger::Info("saving cached-shaders (%u): ", Entry.size());

        LockGuard<Mutex> guard(shaderSourceEntryMutex);
        bool success = true;

        SCOPE_EXIT
        {
            SafeRelease(file);

            if (success)
            {
                FileSystem::Instance()->MoveFile(cacheTempFile, fileName, true);
            }
            else
            {
                FileSystem::Instance()->DeleteFile(cacheTempFile);
            }
        };
        
#define WRITE_CHECK(exp) if (!exp) { success = false; return; }

        WRITE_CHECK(WriteUI4(file, FormatVersion));
        WRITE_CHECK(WriteUI4(file, static_cast<uint32>(Entry.size())));
        for (std::vector<entry_t>::const_iterator e = Entry.begin(), e_end = Entry.end(); e != e_end; ++e)
        {
            WRITE_CHECK(WriteS0(file, e->uid.c_str()));
            WRITE_CHECK(WriteUI4(file, e->api));
            WRITE_CHECK(WriteUI4(file, e->srcHash));
            WRITE_CHECK(e->src->Save(Api(e->api), file));
        }
        
#undef WRITE_CHECK
    }
}

//------------------------------------------------------------------------------

void ShaderSourceCache::Load(const char* fileName)
{
    using namespace DAVA;

    ScopedPtr<File> file(File::Create(fileName, File::READ | File::OPEN));

    if (file)
    {
        Clear();

        bool success = true;
        SCOPE_EXIT
        {
            if (!success)
            {
                Clear();
                Logger::Warning("ShaderSource-Cache failed to load, ignoring cached shaders\n");
            }
        };
        
#define READ_CHECK(exp) if (!exp) { success = false; return; }

        uint32 formatVersion = 0;
        READ_CHECK(ReadUI4(file, &formatVersion));

        if (formatVersion == FormatVersion)
        {
            LockGuard<Mutex> guard(shaderSourceEntryMutex);

            uint32 entryCount = 0;
            READ_CHECK(ReadUI4(file, &entryCount));
            Entry.resize(entryCount);
            Logger::Info("loading cached-shaders (%u): ", Entry.size());

            for (std::vector<entry_t>::iterator e = Entry.begin(), e_end = Entry.end(); e != e_end; ++e)
            {
                std::string str;
                READ_CHECK(ReadS0(file, &str));
                e->uid = FastName(str.c_str());
                READ_CHECK(ReadUI4(file, &e->api));
                READ_CHECK(ReadUI4(file, &e->srcHash));
                e->src = new ShaderSource();

                READ_CHECK(e->src->Load(Api(e->api), file));
            }
        }
        else
        {
            Logger::Warning("ShaderSource-Cache version mismatch, ignoring cached shaders\n");
            success = false;
        }
        
#undef READ_CHECK
    }
}

//==============================================================================
} // namespace rhi
