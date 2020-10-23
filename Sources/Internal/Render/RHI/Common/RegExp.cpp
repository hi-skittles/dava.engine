#include "RegExp.h"
    #include "Base/Platform.h"    

    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
    #include <ctype.h>
    #include <assert.h>
    #if defined(__DAVAENGINE_WIN32__)
    #include <malloc.h>
    #endif

#define _REGEX_STRING(x) x
inline const char* _regex_tcsinc(const char* cur)
{
    return cur + 1;
}
inline char* _regex_tcsinc(char* cur)
{
    return cur + 1;
}
inline char* _regex_tcsdec(char* start, char* cur)
{
    return (cur > start) ? cur - 1 : NULL;
}
inline const char* _regex_tcsdec(const char* start, const char* cur)
{
    return (cur > start) ? cur - 1 : NULL;
}
inline char _regex_tcsnextc(const char* str)
{
    return *str;
}
#if defined(__DAVAENGINE_POSIX__)
#define _regex_tcsdup strdup
#else
#define _regex_tcsdup _strdup
#endif
#define _regex_tcscpy strcpy
#define _regex_tcscpy strcpy
#define _regex_tcslen strlen
#define _regex_tcschr strchr
#define _regex_totupper toupper
#define _regex_totlower tolower
#define _regex_istalpha isalpha
#define _regex_istdigit isdigit
#define _regex_istlower islower

//==============================================================================
//
//  publics:

#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_WIN_UAP__)
#pragma warning(push, 3)
#pragma warning(disable : 174)
#pragma warning(disable : 193)
#else
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif

// Disable debugging printf's
//#define printf    1 ||
//#define printf    1 ||
//#define wprintf   1 ||

//==============================================================================

struct
RegExp::RegBuffer
{
    unsigned char* data = nullptr;
    unsigned offset = 0;
    unsigned size = 0;

    RegBuffer() = default;
    ~RegBuffer();

    RegBuffer(const RegBuffer&) = delete;
    RegBuffer& operator=(const RegBuffer&) = delete;

    void reserve(unsigned nbytes);
    void write(const void* data, unsigned nbytes);
    void writeByte(unsigned b);
    void writeword(unsigned b);
    void writechar(char c);
    void write4(unsigned w);
    void write(RegBuffer* buf);
    void fill0(unsigned nbytes);
    void spread(unsigned offset, unsigned nbytes);
};

//==============================================================================
//
// Opcodes
//

enum REopcodes
{
    REend, // end of program
    REchar, // single character
    REichar, // single character, case insensitive
    REwchar, // single wide character
    REiwchar, // single wide character, case insensitive
    REanychar, // any character
    REanystar, // ".*"
    REstring, // string of characters
    REtestbit, // any in bitmap, non-consuming
    REbit, // any in the bit map
    REnotbit, // any not in the bit map
    RErange, // any in the string
    REnotrange, // any not in the string
    REor, // a | b
    REplus, // 1 or more
    REstar, // 0 or more
    REquest, // 0 or 1
    REnm, // n..m
    REnmq, // n..m, non-greedy version
    REbol, // beginning of line
    REeol, // end of line
    REparen, // parenthesized subexpression
    REgoto, // goto offset

    REwordboundary,
    REnotwordboundary,
    REdigit,
    REnotdigit,
    REspace,
    REnotspace,
    REword,
    REnotword,
    REbackref
};

// BUG: should this include '$'?
static int isword(char c)
{
    return isalnum(c) || c == '_';
}

static unsigned inf = ~0u;

//------------------------------------------------------------------------------

RegExp::RegExp()
{
    memset(this, 0, sizeof(RegExp));
    pmatch = &_match;
}

//------------------------------------------------------------------------------

RegExp::~RegExp()
{
    if (!_is_ref)
        free(_pattern);

    if (pmatch != &_match)
        free(pmatch); //lint !e424 Inappropriate deallocation

    free(_program);
}

//------------------------------------------------------------------------------

bool RegExp::compile(const char* pattern, const char* attributes)
{
    // returns:
    // true, if success;
    // false, if failed.

    if (!attributes)
        attributes = _REGEX_STRING("");
    //    wprintf(L"RegExp::compile('%s', '%s', %d)\n", pattern, attributes, _is_ref);
    this->_attributes = 0;
    if (attributes)
    {
        const char* p = attributes;

        for (; *p; p = _regex_tcsinc(p))
        {
            unsigned att;

            switch (*p)
            {
            case 'g':
                att = attrGlobal;
                break;
            case 'i':
                att = attrIgnoreCase;
                break;
            case 'm':
                att = attrMultiline;
                break;
            default:
                return false; // unrecognized attribute
            }
            if (this->_attributes & att)
                return false; // redundant attribute
            this->_attributes |= att;
        }
    }

    _input = NULL;

    if (!this->_is_ref)
        free(this->_pattern);
    this->_pattern = _regex_tcsdup(pattern);

    // warning : assigning field to itself
    // this->_is_ref = _is_ref;
    _regex_tcscpy(flags, attributes);

    unsigned oldre_nsub = re_nsub;
    re_nsub = 0;
    _error_count = 0;

    RegBuffer regBuffer;
    _buf = &regBuffer;
    _buf->reserve((unsigned)_regex_tcslen(pattern) * 8);
    _parser_pos = this->_pattern;
    _parse_regexp();
    if (*_parser_pos)
    {
        _error("unmatched ')'");
    }
    _optimize();
    _program = (char*)_buf->data;
    _buf->data = nullptr;
    _buf = nullptr;

    if (re_nsub > oldre_nsub)
    {
        if (pmatch == &_match)
            pmatch = NULL;
        pmatch = (Match*)realloc(pmatch, (re_nsub + 1) * sizeof(Match));
    }
    pmatch[0].begin = 0;
    pmatch[0].end = 0;

    return (_error_count == 0) ? true : false;
}

//------------------------------------------------------------------------------

bool RegExp::test(const char* string, int startindex)
{
    //    L_ASSERT(_pattern[0])

    // returns:
    // true, if string passed test;
    // false, if test failed.

    const char* s;
    const char* sstart;
    unsigned firstc;

#if defined _UNICODE
//    wprintf(L"RegExp::test(string = '%s', index = %d)\n", string, _match.end);
#else
//printf("RegExp::test(string = '%s', index = %d)\n", string, _match.rm_eo);
#endif
    pmatch[0].begin = 0;
    pmatch[0].end = 0;
    _input = string;
    if (startindex < 0 || startindex > int(_regex_tcslen(string)))
    {
        return false; // fail
    }
    sstart = string + startindex;

    _print_program(_program);

    // First character optimization
    firstc = 0;
    if (_program[0] == REchar)
    {
        firstc = *(unsigned char*)(_program + 1);
        if (_attributes & attrIgnoreCase && isalpha(firstc))
            firstc = 0;
    }

    for (s = sstart;; s = _regex_tcsinc(s))
    {
        if (firstc && _regex_tcsnextc(s) != (char)firstc)
        {
            s = _regex_tcsinc(s);
            s = _regex_tcschr(s, firstc);
            if (!s)
                break;
        }
        memset(pmatch, -1, (re_nsub + 1) * sizeof(Match));
        _src_start = _src = s;
        if (_try_match(_program, NULL))
        {
            pmatch[0].begin = static_cast<int>(_src_start - _input);
            pmatch[0].end = static_cast<int>(_src - _input);
            return true;
        }
        // If possible _match must start at beginning, we are done
        if (_program[0] == REbol || _program[0] == REanystar)
        {
            if (_attributes & attrMultiline)
            {
                // Scan for the next \n
                s = _regex_tcschr(s, '\n');
                if (!s)
                    break;
            }
            else
                break;
        }
        if (!*s)
            break;
    }

    return false; // no _match
}

//------------------------------------------------------------------------------

void RegExp::_print_program(char* /*prog*/)
{
    /*
    char *progstart;
    int pc;
    unsigned len;
    unsigned n;
    unsigned m;

    printf("_print_program()\n");
    progstart = prog;
    for (;;)
    {
    pc = prog - progstart;
    printf("%3d: ", pc);

    switch (*prog)
    {
        case REchar:
        wprintf(L"\tREchar '%c'\n", *(unsigned char *)(prog + 1));
        prog += 1 + sizeof(char);
        break;

        case REichar:
        printf("\tREichar '%c'\n", *(unsigned char *)(prog + 1));
        prog += 1 + sizeof(char);
        break;

        case REwchar:
        printf("\tREwchar '%c'\n", *(wchar_t *)(prog + 1));
        prog += 1 + sizeof(wchar_t);
        break;

        case REiwchar:
        printf("\tREiwchar '%c'\n", *(wchar_t *)(prog + 1));
        prog += 1 + sizeof(wchar_t);
        break;

        case REanychar:
        printf("\tREanychar\n");
        prog++;
        break;

        case REstring:
        len = *(unsigned *)(prog + 1);
        wprintf(L"\tREstring x%x, '%c'\n", len,
            *(char *)(prog + 1 + sizeof(unsigned)));
        prog += 1 + sizeof(unsigned) + len * sizeof(char);
        break;

        case REtestbit:
        printf("\tREtestbit %d, %d\n",
            ((unsigned short *)(prog + 1))[0],
            ((unsigned short *)(prog + 1))[1]);
        len = ((unsigned short *)(prog + 1))[1];
        prog += 1 + 2 * sizeof(unsigned short) + len;
        break;

        case REbit:
        printf("\tREbit %d, %d\n",
            ((unsigned short *)(prog + 1))[0],
            ((unsigned short *)(prog + 1))[1]);
        len = ((unsigned short *)(prog + 1))[1];
        prog += 1 + 2 * sizeof(unsigned short) + len;
        break;

        case REnotbit:
        printf("\tREnotbit %d, %d\n",
            ((unsigned short *)(prog + 1))[0],
            ((unsigned short *)(prog + 1))[1]);
        len = ((unsigned short *)(prog + 1))[1];
        prog += 1 + 2 * sizeof(unsigned short) + len;
        break;

        case RErange:
        printf("\tRErange %d\n", *(unsigned *)(prog + 1));
        // BUG: attrIgnoreCase?
        len = *(unsigned *)(prog + 1);
        prog += 1 + sizeof(unsigned) + len;
        break;

        case REnotrange:
        printf("\tREnotrange %d\n", *(unsigned *)(prog + 1));
        // BUG: attrIgnoreCase?
        len = *(unsigned *)(prog + 1);
        prog += 1 + sizeof(unsigned) + len;
        break;

        case REbol:
        printf("\tREbol\n");
        prog++;
        break;

        case REeol:
        printf("\tREeol\n");
        prog++;
        break;

        case REor:
        len = ((unsigned *)(prog + 1))[0];
        printf("\tREor %d, pc=>%d\n", len, pc + 1 + sizeof(unsigned) + len);
        prog += 1 + sizeof(unsigned);
        break;

        case REgoto:
        len = ((unsigned *)(prog + 1))[0];
        printf("\tREgoto %d, pc=>%d\n", len, pc + 1 + sizeof(unsigned) + len);
        prog += 1 + sizeof(unsigned);
        break;

        case REanystar:
        printf("\tREanystar\n");
        prog++;
        break;

        case REnm:
        case REnmq:
        // len, n, m, ()
        len = ((unsigned *)(prog + 1))[0];
        n = ((unsigned *)(prog + 1))[1];
        m = ((unsigned *)(prog + 1))[2];
        wprintf(L"\tREnm%s len=%d, n=%u, m=%u, pc=>%d\n", (*prog == REnmq) ? L"q" : L"", len, n, m, pc + 1 + sizeof(unsigned) * 3 + len);
        prog += 1 + sizeof(unsigned) * 3;
        break;

        case REparen:
        // len, ()
        len = ((unsigned *)(prog + 1))[0];
        n = ((unsigned *)(prog + 1))[1];
        printf("\tREparen len=%d n=%d, pc=>%d\n", len, n, pc + 1 + sizeof(unsigned) * 2 + len);
        prog += 1 + sizeof(unsigned) * 2;
        break;

        case REend:
        printf("\tREend\n");
        return;

        case REwordboundary:
        printf("\tREwordboundary\n");
        prog++;
        break;

        case REnotwordboundary:
        printf("\tREnotwordboundary\n");
        prog++;
        break;

        case REdigit:
        printf("\tREdigit\n");
        prog++;
        break;

        case REnotdigit:
        printf("\tREnotdigit\n");
        prog++;
        break;

        case REspace:
        printf("\tREspace\n");
        prog++;
        break;

        case REnotspace:
        printf("\tREnotspace\n");
        prog++;
        break;

        case REword:
        printf("\tREword\n");
        prog++;
        break;

        case REnotword:
        printf("\tREnotword\n");
        prog++;
        break;

        case REbackref:
        printf("\tREbackref %d\n", prog[1]);
        prog += 2;
        break;

        default:
        assert(0);
    }
    }
*/
}

//------------------------------------------------------------------------------

int RegExp::_try_match(char* prog, char* progend)
{
    const char* srcsave;
    unsigned len;
    unsigned n;
    unsigned m;
    unsigned count;
    char* pop;
    const char* ss;
    Match* psave;
    unsigned c1;
    unsigned c2;

    srcsave = _src;
    psave = NULL;
    for (;;)
    {
        if (prog == progend) // if done matching
        {
            //      printf( "\tprogend\n" );
            return 1;
        }

        //printf("\top = %d\n", *prog);
        switch (*prog)
        {
        case REchar:
            if (*(unsigned char*)(prog + 1) != _regex_tcsnextc(_src))
                goto Lnomatch;
            _src = _regex_tcsinc(_src);
            prog += 1 + sizeof(char);
            break;

        case REichar:
            c1 = *(unsigned char*)(prog + 1);
            c2 = _regex_tcsnextc(_src);
            if (c1 != c2)
            {
                if (_regex_istlower((char)c2))
                    c2 = _regex_totupper((char)c2);
                else
                    goto Lnomatch;
                if (c1 != c2)
                    goto Lnomatch;
            }
            _src = _regex_tcsinc(_src);
            prog += 1 + sizeof(char);
            break;

        case REwchar:
            if (*(wchar_t*)(prog + 1) != _regex_tcsnextc(_src))
                goto Lnomatch;
            _src = _regex_tcsinc(_src);
            prog += 1 + sizeof(wchar_t);
            break;

        case REiwchar:
            c1 = *(wchar_t*)(prog + 1);
            c2 = _regex_tcsnextc(_src);
            if (c1 != c2)
            {
                if (_regex_istlower((char)c2))
                    c2 = _regex_totupper((char)c2);
                else
                    goto Lnomatch;
                if (c1 != c2)
                    goto Lnomatch;
            }
            _src = _regex_tcsinc(_src);
            prog += 1 + sizeof(wchar_t);
            break;

        case REanychar:
            if (!*_src)
                goto Lnomatch;
            if (!(_attributes & attrDotMatchLF) && *_src == '\n')
                goto Lnomatch;
            _src = _regex_tcsinc(_src);
            prog++;
            break;

        case REstring:
            //wprintf(L"\tREstring x%x, '%c'\n", *(unsigned *)(prog + 1),
            //  *(char *)(prog + 1 + sizeof(unsigned)));
            len = *(unsigned*)(prog + 1);
            if (memcmp(prog + 1 + sizeof(unsigned), _src, len * sizeof(char)))
                goto Lnomatch;
            _src += len;
            prog += 1 + sizeof(unsigned) + len * sizeof(char);
            break;

        case REtestbit:
            len = ((unsigned short*)(prog + 1))[1];
            c1 = _regex_tcsnextc(_src);
            //printf("[x%02x]=x%02x, x%02x\n", c1 >> 3, ((prog + 1 + 4)[c1 >> 3] ), (1 << (c1 & 7)));
            if (c1 <= ((unsigned short*)(prog + 1))[0] &&
                !((prog + 1 + 4)[c1 >> 3] & (1 << (c1 & 7))))
                goto Lnomatch;
            prog += 1 + 2 * sizeof(unsigned short) + len;
            break;

        case REbit:
            len = ((unsigned short*)(prog + 1))[1];
            c1 = _regex_tcsnextc(_src);
            if (c1 > ((unsigned short*)(prog + 1))[0])
                goto Lnomatch;
            if (!((prog + 1 + 4)[c1 >> 3] & (1 << (c1 & 7))))
                goto Lnomatch;
            _src = _regex_tcsinc(_src);
            prog += 1 + 2 * sizeof(unsigned short) + len;
            break;

        case REnotbit:
            len = ((unsigned short*)(prog + 1))[1];
            c1 = _regex_tcsnextc(_src);
            if (c1 <= ((unsigned short*)(prog + 1))[0] &&
                ((prog + 1 + 4)[c1 >> 3] & (1 << (c1 & 7))))
                goto Lnomatch;
            _src = _regex_tcsinc(_src);
            prog += 1 + 2 * sizeof(unsigned short) + len;
            break;

        case RErange:
            // BUG: attrIgnoreCase?
            len = *(unsigned*)(prog + 1);
            if (memchr(prog + 1 + sizeof(unsigned), _regex_tcsnextc(_src), len) == NULL)
                goto Lnomatch;
            _src = _regex_tcsinc(_src);
            prog += 1 + sizeof(unsigned) + len;
            break;

        case REnotrange:
            // BUG: attrIgnoreCase?
            len = *(unsigned*)(prog + 1);
            if (memchr(prog + 1 + sizeof(unsigned), _regex_tcsnextc(_src), len) != NULL)
                goto Lnomatch;
            _src = _regex_tcsinc(_src);
            prog += 1 + sizeof(unsigned) + len;
            break;

        case REbol:
            if (_src == _input)
                ;
            else if (_attributes & attrMultiline)
            {
                char* p;

                p = (char*)_regex_tcsdec(_input, _src);
                if (_regex_tcsnextc(p) != '\n')
                    goto Lnomatch;
            }
            else
                goto Lnomatch;
            prog++;
            break;

        case REeol:
            if (*_src == 0)
                ;
            else if (_attributes & attrMultiline && *_src == '\n')
                _src = _regex_tcsinc(_src);
            else
                goto Lnomatch;
            prog++;
            break;

        case REor:
            len = ((unsigned*)(prog + 1))[0];
            pop = prog + 1 + sizeof(unsigned);
            ss = _src;
            if (_try_match(pop, progend))
            {
                if (progend)
                {
                    const char* s;

                    s = _src;
                    if (_try_match(progend, NULL))
                    {
                        _src = s;
                        return 1;
                    }
                    else
                    {
                        // If second branch doesn't _match to end, take first anyway
                        _src = ss;
                        if (!_try_match(pop + len, NULL))
                        {
                            _src = s;
                            return 1;
                        }
                    }
                    _src = ss;
                }
                else
                {
                    return 1;
                }
            }
            prog = pop + len; // proceed with 2nd branch
            break;

        case REgoto:
            len = ((unsigned*)(prog + 1))[0];
            prog += 1 + sizeof(unsigned) + len;
            break;

        case REanystar:
            prog++;
            for (;;)
            {
                const char* s1;
                const char* s2;

                s1 = _src;
                if (!*_src)
                    break;
                if (!(_attributes & attrDotMatchLF) && *_src == '\n')
                    break;
                _src = _regex_tcsinc(_src);
                s2 = _src;

                // If no _match after consumption, but it
                // did match before, then no match
                if (!_try_match(prog, NULL))
                {
                    _src = s1;
                    // BUG: should we save/restore pmatch[]?
                    if (_try_match(prog, NULL))
                    {
                        _src = s1; // no match
                        break;
                    }
                }
                _src = s2;
            }
            break;

        case REnm:
        case REnmq:
            // len, n, m, ()
            len = ((unsigned*)(prog + 1))[0];
            n = ((unsigned*)(prog + 1))[1];
            m = ((unsigned*)(prog + 1))[2];
            pop = prog + 1 + sizeof(unsigned) * 3;
            for (count = 0; count < n; count++)
            {
                if (!_try_match(pop, pop + len))
                    goto Lnomatch;
            }

            if (!psave && count < m)
            {
                psave = (Match*)alloca((re_nsub + 1) * sizeof(Match));
            }

            if (*prog == REnmq) // if minimal munch
            {
                for (; count < m; count++)
                {
                    const char* s1;

                    memcpy(psave, pmatch, (re_nsub + 1) * sizeof(Match));
                    s1 = _src;

                    if (_try_match(pop + len, NULL))
                    {
                        _src = s1;
                        memcpy(pmatch, psave, (re_nsub + 1) * sizeof(Match));
                        break;
                    }

                    if (!_try_match(pop, pop + len))
                    {
                        break;
                    }

                    // If source is not consumed, don't
                    // infinite loop on the match
                    if (s1 == _src)
                    {
                        break;
                    }
                }
            }
            else // maximal munch
            {
                for (; count < m; count++)
                {
                    const char* s1;
                    const char* s2;

                    memcpy(psave, pmatch, (re_nsub + 1) * sizeof(Match));
                    s1 = _src;
                    if (!_try_match(pop, pop + len))
                    {
                        break;
                    }
                    s2 = _src;

                    // If source is not consumed, don't
                    // infinite loop on the match
                    if (s1 == s2)
                    {
                        break;
                    }

                    // If no match after consumption, but it
                    // did match before, then no match
                    if (!_try_match(pop + len, NULL))
                    {
                        _src = s1;
                        if (_try_match(pop + len, NULL))
                        {
                            _src = s1; // no match
                            memcpy(pmatch, psave, (re_nsub + 1) * sizeof(Match));
                            break;
                        }
                    }
                    _src = s2;
                }
            }
            prog = pop + len;
            break;

        case REparen:
            // len, ()
            len = ((unsigned*)(prog + 1))[0];
            n = ((unsigned*)(prog + 1))[1];
            pop = prog + 1 + sizeof(unsigned) * 2;
            ss = _src;
            if (!_try_match(pop, pop + len))
                goto Lnomatch;
            pmatch[n + 1].begin = static_cast<int>(ss - _input);
            pmatch[n + 1].end = static_cast<int>(_src - _input);
            prog = pop + len;
            break;

        case REend:
            return 1; // successful match

        case REwordboundary:
            if (_src != _input)
            {
                c1 = _regex_tcsnextc(_regex_tcsdec(_input, _src));
                c2 = _regex_tcsnextc(_src);
                if (!(
                    (isword((char)c1) && !isword((char)c2)) ||
                    (!isword((char)c1) && isword((char)c2))))
                    goto Lnomatch;
            }
            prog++;
            break;

        case REnotwordboundary:
            if (_src == _input)
                goto Lnomatch;
            c1 = _regex_tcsnextc(_regex_tcsdec(_input, _src));
            c2 = _regex_tcsnextc(_src);
            if (
            (isword((char)c1) && !isword((char)c2)) ||
            (!isword((char)c1) && isword((char)c2)))
                goto Lnomatch;
            prog++;
            break;

        case REdigit:
            if (!_regex_istdigit(_regex_tcsnextc(_src)))
                goto Lnomatch;
            _src = _regex_tcsinc(_src);
            prog++;
            break;

        case REnotdigit:
            c1 = _regex_tcsnextc(_src);
            if (!c1 || _regex_istdigit((char)c1))
                goto Lnomatch;
            _src = _regex_tcsinc(_src);
            prog++;
            break;

        case REspace:
            if (!isspace(_regex_tcsnextc(_src)))
                goto Lnomatch;
            _src = _regex_tcsinc(_src);
            prog++;
            break;

        case REnotspace:
            c1 = _regex_tcsnextc(_src);
            if (!c1 || isspace(c1))
                goto Lnomatch;
            _src = _regex_tcsinc(_src);
            prog++;
            break;

        case REword:
            if (!isword(_regex_tcsnextc(_src)))
                goto Lnomatch;
            _src = _regex_tcsinc(_src);
            prog++;
            break;

        case REnotword:
            c1 = _regex_tcsnextc(_src);
            if (!c1 || isword((char)c1))
                goto Lnomatch;
            _src = _regex_tcsinc(_src);
            prog++;
            break;

        case REbackref:
            n = prog[1];
            len = pmatch[n + 1].end - pmatch[n + 1].begin;
            if (_attributes & attrIgnoreCase)
            {
#if defined(__DAVAENGINE_WIN32__)
                if (_memicmp(_src, _input + pmatch[n + 1].begin, len))
                    goto Lnomatch;
#endif
            }
            else if (memcmp(_src, _input + pmatch[n + 1].begin, len * sizeof(char)))
                goto Lnomatch;

            _src += len;
            prog += 2;
            break;

            ///        default:
            ///        L_ASSERTMSG("");
        }
    }

Lnomatch:
    _src = srcsave;
    return 0;
}

//==============================================================================
//
//  Compiler
//

int RegExp::_parse_regexp()
{
    unsigned offset;
    unsigned gotooffset;
    unsigned len1;
    unsigned len2;

    offset = _buf->offset;
    for (;;)
    {
        switch (*_parser_pos)
        {
        case ')':
            return 1;

        case 0:
            _buf->writeByte(REend);
            return 1;

        case '|':
            _parser_pos++;
            gotooffset = _buf->offset;
            _buf->writeByte(REgoto);
            _buf->write4(0);
            len1 = _buf->offset - offset;
            _buf->spread(offset, 1 + sizeof(unsigned));
            gotooffset += 1 + sizeof(unsigned);
            _parse_regexp();
            len2 = _buf->offset - (gotooffset + 1 + sizeof(unsigned));
            _buf->data[offset] = REor;
            ((unsigned*)(_buf->data + offset + 1))[0] = len1;
            ((unsigned*)(_buf->data + gotooffset + 1))[0] = len2;
            break;

        default:
            _parse_piece();
            break;
        }
    }
}

//------------------------------------------------------------------------------

int RegExp::_parse_piece()
{
    unsigned offset;
    unsigned len;
    unsigned n;
    unsigned m;
    char op;

    offset = _buf->offset;
    _parse_atom();
    switch (*_parser_pos)
    {
    case '*':
        // Special optimization: replace .* with REanystar
        if (_buf->offset - offset == 1 &&
            _buf->data[offset] == REanychar &&
            _parser_pos[1] != '?')
        {
            _buf->data[offset] = REanystar;
            _parser_pos++;
            break;
        }

        n = 0;
        m = inf;
        goto Lnm;

    case '+':
        n = 1;
        m = inf;
        goto Lnm;

    case '?':
        n = 0;
        m = 1;
        goto Lnm;

    case '{': // {n} {n,} {n,m}
        _parser_pos++;
        if (!_regex_istdigit(*_parser_pos))
            goto Lerr;
        n = 0;
        do
        {
            // BUG: handle overflow
            n = n * 10 + *_parser_pos - '0';
            _parser_pos++;
        } while (_regex_istdigit(*_parser_pos));
        if (*_parser_pos == '}') // {n}
        {
            m = n;
            goto Lnm;
        }
        if (*_parser_pos != ',')
            goto Lerr;
        _parser_pos++;
        if (*_parser_pos == '}') // {n,}
        {
            m = inf;
            goto Lnm;
        }
        if (!_regex_istdigit(*_parser_pos))
            goto Lerr;
        m = 0; // {n,m}
        do
        {
            // BUG: handle overflow
            m = m * 10 + *_parser_pos - '0';
            _parser_pos++;
        } while (_regex_istdigit(*_parser_pos));
        if (*_parser_pos != '}')
            goto Lerr;
        goto Lnm;

    Lnm:
        _parser_pos++;
        op = REnm;
        if (*_parser_pos == '?')
        {
            op = REnmq; // minimal munch version
            _parser_pos++;
        }
        len = _buf->offset - offset;
        _buf->spread(offset, 1 + sizeof(unsigned) * 3);
        _buf->data[offset] = op;

        // we are using temporary buffer + memcpy to prevent
        // unaligned memory access on ARM devices
        unsigned localBuffer[3] = { len, n, m };
        memcpy(_buf->data + offset + 1, localBuffer, sizeof(localBuffer));
        break;
    }
    return 1;

Lerr:
    _error("badly formed {n,m}");
    return 0;
}

//------------------------------------------------------------------------------

int RegExp::_parse_atom()
{
    char op;
    unsigned offset;
    unsigned c;

    c = *_parser_pos;
    switch (c)
    {
    case '*':
    case '+':
    case '?':
        _error("*+? not allowed in atom");
        _parser_pos++;
        return 0;

    case '(':
        _parser_pos++;
        _buf->writeByte(REparen);
        offset = _buf->offset;
        _buf->write4(0); // reserve space for length
        _buf->write4(re_nsub);
        re_nsub++;
        _parse_regexp();
        *(unsigned*)(_buf->data + offset) =
        _buf->offset - (offset + sizeof(unsigned) * 2);
        if (*_parser_pos != ')')
        {
            _error("')' expected");
            return 0;
        }
        _parser_pos++;
        break;

    case '[':
        if (!_parse_range())
            return 0;
        break;

    case '.':
        _parser_pos++;
        _buf->writeByte(REanychar);
        break;

    case '^':
        _parser_pos++;
        _buf->writeByte(REbol);
        break;

    case '$':
        _parser_pos++;
        _buf->writeByte(REeol);
        break;

    case 0:
        break;

    case '\\':
        _parser_pos++;
        switch (*_parser_pos)
        {
        case 0:
            _error("no character past '\\'");
            return 0;

        case 'b':
            op = REwordboundary;
            goto Lop;
        case 'B':
            op = REnotwordboundary;
            goto Lop;
        case 'd':
            op = REdigit;
            goto Lop;
        case 'D':
            op = REnotdigit;
            goto Lop;
        case 's':
            op = REspace;
            goto Lop;
        case 'S':
            op = REnotspace;
            goto Lop;
        case 'w':
            op = REword;
            goto Lop;
        case 'W':
            op = REnotword;
            goto Lop;

        Lop:
            _buf->writeByte(op);
            _parser_pos++;
            break;

        case 'f':
        case 'n':
        case 'r':
        case 't':
        case 'v':
        case 'c':
        case 'x':
        case 'u':
        case '0':
            c = _escape();
            goto Lbyte;

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            c = *_parser_pos - '1';
            if (c < re_nsub)
            {
                _buf->writeByte(REbackref);
                _buf->writeByte(c);
            }
            else
            {
                _error("no matching back reference");
                return 0;
            }
            _parser_pos++;
            break;

        default:
            c = _regex_tcsnextc(_parser_pos);
            _parser_pos = _regex_tcsinc(_parser_pos);
            goto Lbyte;
        }
        break;

    default:
        c = _regex_tcsnextc(_parser_pos);
        _parser_pos = _regex_tcsinc(_parser_pos);
    Lbyte:
        op = REchar;
        if (_attributes & attrIgnoreCase)
        {
            if (_regex_istalpha((char)c))
            {
                op = REichar;
                c = _regex_totupper((char)c);
            }
        }
        if (op == REchar && c <= 0xFF)
        {
            // Look ahead and see if we can make this into
            // an REstring
            char* q;
            int len;

            for (q = _parser_pos;; q = _regex_tcsinc(q))
            {
                char qc = (char)_regex_tcsnextc(q);

                switch (qc)
                {
                case '{':
                case '*':
                case '+':
                case '?':
                    if (q == _parser_pos)
                        goto Lchar;
                    q--;
                    break;

                case '(':
                case ')':
                case '|':
                case '[':
                case ']':
                case '.':
                case '^':
                case '$':
                case '\\':
                case '}':
                case 0:
                    break;

                default:
                    continue;
                }
                break;
            }
            len = static_cast<int>(q - _parser_pos);
            if (len > 0)
            {
                _buf->reserve(5 + (1 + len) * sizeof(char));
                _buf->writeByte(REstring);
                _buf->write4(len + 1);
                _buf->writechar(c);
                _buf->write(_parser_pos, len * sizeof(char));
                _parser_pos = q;
                break;
            }
        }
        if (c & ~0xFF)
        {
            // Convert to 16 bit opcode
            op = (char)((op == REchar) ? REwchar : REiwchar);
            _buf->writeByte(op);
            _buf->writeword(c);
        }
        else
        {
        Lchar:
            _buf->writeByte(op);
            _buf->writeByte(c);
        }
        break;
    }
    return 1;
}

//------------------------------------------------------------------------------

struct
RegExp::Range
{
    unsigned maxc;
    unsigned maxb;
    RegBuffer* buf;
    unsigned char* base;

    Range(RegBuffer* b)
        : maxc(0)
        , maxb(0)
        , buf(b)
        , base(&b->data[b->offset])
    {
    }

    void setbitmax(unsigned u)
    {
        unsigned b;

        if (u > maxc)
        {
            maxc = u;
            b = u >> 3;

            if (b >= maxb)
            {
                unsigned uu;

                uu = static_cast<unsigned>(base - buf->data);
                buf->fill0(b - maxb + 1);
                base = buf->data + uu;
                maxb = b + 1;
            }
        }
    }

    void setbit(unsigned u)
    {
        base[u >> 3] |= 1 << (u & 7);
    }

    void setbit2(unsigned u)
    {
        setbitmax(u + 1);
        base[u >> 3] |= 1 << (u & 7);
    }

    int testbit(unsigned u)
    {
        return base[u >> 3] & (1 << (u & 7));
    }

    void negate()
    {
        for (unsigned b = 0; b < maxb; b++)
            base[b] = (unsigned char)~base[b];
    }
};

//------------------------------------------------------------------------------

int RegExp::_parse_range()
{
    char op;
    int c = -1; // initialize to keep /W4 happy
    int c2 = -1; // initialize to keep /W4 happy
    unsigned i;
    unsigned cmax;
    unsigned offset;

    cmax = 0x7F;
    _parser_pos++;
    op = REbit;
    if (*_parser_pos == '^')
    {
        _parser_pos++;
        op = REnotbit;
    }
    _buf->writeByte(op);
    offset = _buf->offset;
    _buf->write4(0); // reserve space for length
    _buf->reserve(128 / 8);
    Range r(_buf);
    if (op == REnotbit)
        r.setbit2(0);
    switch (*_parser_pos)
    {
    case ']':
    case '-':
        c = *_parser_pos;
        _parser_pos = _regex_tcsinc(_parser_pos);
        r.setbit2(c);
        break;
    }

    enum RangeState
    {
        RSstart,
        RSrliteral,
        RSdash
    };
    RangeState rs;

    rs = RSstart;
    for (;;)
    {
        //lint -e{616} // control flows into case/default
        switch (*_parser_pos)
        {
        case ']':
            //lint -e{616} // control flows into case/default
            switch (rs)
            {
            case RSdash:
                r.setbit2('-');
            case RSrliteral:
                r.setbit2(c);
                break;
            default:
                break;
            }
            _parser_pos++;
            break;

        case '\\':
            _parser_pos++;
            r.setbitmax(cmax);
            switch (*_parser_pos)
            {
            case 'd':
                for (i = '0'; i <= '9'; i++)
                    r.setbit(i);
                goto Lrs;

            case 'D':
                for (i = 1; i < '0'; i++)
                    r.setbit(i);
                for (i = '9' + 1; i <= cmax; i++)
                    r.setbit(i);
                goto Lrs;

            case 's':
                for (i = 0; i <= cmax; i++)
                    if (isspace(i))
                        r.setbit(i);
                goto Lrs;

            case 'S':
                for (i = 1; i <= cmax; i++)
                    if (!isspace(i))
                        r.setbit(i);
                goto Lrs;

            case 'w':
                for (i = 0; i <= cmax; i++)
                    if (isword((char)i))
                        r.setbit(i);
                goto Lrs;

            case 'W':
                for (i = 1; i <= cmax; i++)
                    if (!isword((char)i))
                        r.setbit(i);
                goto Lrs;

            Lrs:
                switch (rs)
                {
                case RSdash:
                    r.setbit2('-');
                case RSrliteral:
                    r.setbit2(c);
                    break;
                default:
                    break;
                }
                rs = RSstart;
                continue;
            }
            c2 = _escape();
            goto Lrange;

        case '-':
            _parser_pos++;
            if (rs == RSstart)
                goto Lrange;
            else if (rs == RSrliteral)
                rs = RSdash;
            else if (rs == RSdash)
            {
                r.setbit2(c);
                r.setbit2('-');
                rs = RSstart;
            }
            continue;

        case 0:
            _error("']' expected");
            return 0;

        default:
            c2 = _regex_tcsnextc(_parser_pos);
            _parser_pos = _regex_tcsinc(_parser_pos);
        Lrange:
            switch (rs)
            {
            case RSrliteral:
                r.setbit2(c);
            case RSstart:
                c = c2;
                rs = RSrliteral;
                break;

            case RSdash:
                if (c > c2)
                {
                    _error("inverted range in character class");
                    return 0;
                }
                r.setbitmax(c2);
                for (; c <= c2; c++)
                    r.setbit(c);
                rs = RSstart;
                break;
            }
            continue;
        }
        break;
    }
    ((unsigned short*)(_buf->data + offset))[0] = (unsigned short)r.maxc;
    ((unsigned short*)(_buf->data + offset))[1] = (unsigned short)r.maxb;
    if (_attributes & attrIgnoreCase)
    {
        // BUG: what about unicode?
        r.setbitmax(0x7F);
        for (c = 'a'; c <= 'z'; c++)
        {
            if (r.testbit(c))
                r.setbit(c + 'A' - 'a');
            else if (r.testbit(c + 'A' - 'a'))
                r.setbit(c);
        }
    }
    return 1;
}

//------------------------------------------------------------------------------

void RegExp::_error(const char* msg)
{
    (void)msg; // satisfy /W4
    _parser_pos += _regex_tcslen(_parser_pos); // advance to terminating 0
    _error_count++;
}

//------------------------------------------------------------------------------

int RegExp::_escape()
{
    // _parser_pos is following the \ char

    int c;
    int i;

    c = *_parser_pos; // none of the cases are multibyte
    switch (c)
    {
    case 'b':
        c = '\b';
        break;
    case 'f':
        c = '\f';
        break;
    case 'n':
        c = '\n';
        break;
    case 'r':
        c = '\r';
        break;
    case 't':
        c = '\t';
        break;
    case 'v':
        c = '\v';
        break;

    // BUG: Perl does \a and \e too, should we?

    case 'c':
        _parser_pos = _regex_tcsinc(_parser_pos);
        c = _regex_tcsnextc(_parser_pos);
        // Note: we are deliberately not allowing Unicode letters
        if (!(('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')))
        {
            _error("letter expected following \\c");
            return 0;
        }
        c &= 0x1F;
        break;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
        c -= '0';
        for (i = 0; i < 2; i++)
        {
            _parser_pos++;
            if ('0' <= *_parser_pos && *_parser_pos <= '7')
            {
                c = c * 8 + (*_parser_pos - '0');
                // Treat overflow as if last
                // digit was not an octal digit
                if (c >= 0xFF)
                {
                    c >>= 3;
                    return c;
                }
            }
            else
                return c;
        }
        break;

    case 'x':
        c = 0;
        for (i = 0; i < 2; i++)
        {
            _parser_pos++;
            if ('0' <= *_parser_pos && *_parser_pos <= '9')
                c = c * 16 + (*_parser_pos - '0');
            else if ('a' <= *_parser_pos && *_parser_pos <= 'f')
                c = c * 16 + (*_parser_pos - 'a' + 10);
            else if ('A' <= *_parser_pos && *_parser_pos <= 'F')
                c = c * 16 + (*_parser_pos - 'A' + 10);
            else if (i == 0) // if no hex digits after \x
            {
                // Not a valid \xXX sequence
                return 'x';
            }
            else
                return c;
        }
        break;

    case 'u':
        c = 0;
        for (i = 0; i < 4; i++)
        {
            _parser_pos++;
            if ('0' <= *_parser_pos && *_parser_pos <= '9')
                c = c * 16 + (*_parser_pos - '0');
            else if ('a' <= *_parser_pos && *_parser_pos <= 'f')
                c = c * 16 + (*_parser_pos - 'a' + 10);
            else if ('A' <= *_parser_pos && *_parser_pos <= 'F')
                c = c * 16 + (*_parser_pos - 'A' + 10);
            else
            {
                // Not a valid \uXXXX sequence
                _parser_pos -= i;
                return 'u';
            }
        }
        break;

    default:
        c = _regex_tcsnextc(_parser_pos);
        _parser_pos = _regex_tcsinc(_parser_pos);
        return c;
    }
    _parser_pos++;
    return c;
}

//==============================================================================
//
// Optimizer
//

void RegExp::_optimize()
{
    char* prog;

    prog = (char*)_buf->data;
    for (;;)
    {
        switch (*prog)
        {
        case REend:
        case REanychar:
        case REanystar:
        case REbackref:
        case REeol:
        case REchar:
        case REichar:
        case REwchar:
        case REiwchar:
        case REstring:
        case REtestbit:
        case REbit:
        case REnotbit:
        case RErange:
        case REnotrange:
        case REwordboundary:
        case REnotwordboundary:
        case REdigit:
        case REnotdigit:
        case REspace:
        case REnotspace:
        case REword:
        case REnotword:
            return;

        case REbol:
            prog++;
            continue;

        case REor:
        case REnm:
        case REnmq:
        case REparen:
        case REgoto:
        {
            RegBuffer bitbuf;
            unsigned offset;
            Range r(&bitbuf);

            offset = static_cast<unsigned>(prog - (char*)_buf->data);
            if (_start_chars(&r, prog, NULL))
            {
                _buf->spread(offset, 1 + 4 + r.maxb);
                _buf->data[offset] = REtestbit;
                ((unsigned short*)(_buf->data + offset + 1))[0] = (unsigned short)r.maxc;
                ((unsigned short*)(_buf->data + offset + 1))[1] = (unsigned short)r.maxb;
                memcpy(_buf->data + offset + 1 + 4, r.base, r.maxb);
            }
            return;
        }
            ///        default:
            ///        L_ASSERTMSG("");
        }
    }
}

/////////////////////////////////////////
// OR the leading character bits into r.
// Limit the character range from 0..7F,
// trymatch() will allow through anything over maxc.
// Return 1 if success, 0 if we can't build a filter or
// if there is no point to one.

int RegExp::_start_chars(Range* r, char* prog, char* progend)
{
    unsigned c;
    unsigned maxc;
    unsigned maxb;
    unsigned len;
    unsigned b;
    unsigned n;
    //    unsigned m;
    char* pop;

    for (;;)
    {
        if (prog == progend)
            return 1;
        switch (*prog)
        {
        case REchar:
            c = *(unsigned char*)(prog + 1);
            if (c <= 0x7F)
                r->setbit2(c);
            return 1;

        case REichar:
            c = *(unsigned char*)(prog + 1);
            if (c <= 0x7F)
            {
                r->setbit2(c);
                r->setbit2(_regex_totlower((char)c));
            }
            return 1;

        case REwchar:
        case REiwchar:
            return 1;

        case REanychar:
            return 0; // no point

        case REstring:
            len = *(unsigned*)(prog + 1);
            ///        L_ASSERT(len);
            c = *(char*)(prog + 1 + sizeof(unsigned));
            if (c <= 0x7F)
                r->setbit2(c);
            return 1;

        case REtestbit:
        case REbit:
            maxc = ((unsigned short*)(prog + 1))[0];
            maxb = ((unsigned short*)(prog + 1))[1];
            if (maxc <= 0x7F)
                r->setbitmax(maxc);
            else
                maxb = r->maxb;
            for (b = 0; b < maxb; b++)
                r->base[b] |= *(char*)(prog + 1 + 4 + b);
            return 1;

        case REnotbit:
            maxc = ((unsigned short*)(prog + 1))[0];
            maxb = ((unsigned short*)(prog + 1))[1];
            if (maxc <= 0x7F)
                r->setbitmax(maxc);
            else
                maxb = r->maxb;
            for (b = 0; b < maxb; b++)
                r->base[b] |= ~*(char*)(prog + 1 + 4 + b);
            return 1;

        case REbol:
        case REeol:
            return 0;

        case REor:
            len = ((unsigned*)(prog + 1))[0];
            pop = prog + 1 + sizeof(unsigned);
            return _start_chars(r, pop, progend) &&
            _start_chars(r, pop + len, progend);

        case REgoto:
            len = ((unsigned*)(prog + 1))[0];
            prog += 1 + sizeof(unsigned) + len;
            break;

        case REanystar:
            return 0;

        case REnm:
        case REnmq:
            // len, n, m, ()
            len = ((unsigned*)(prog + 1))[0];
            n = ((unsigned*)(prog + 1))[1];
            //      m = ((unsigned *)(prog + 1))[2];
            pop = prog + 1 + sizeof(unsigned) * 3;
            if (!_start_chars(r, pop, pop + len))
                return 0;
            if (n)
                return 1;
            prog = pop + len;
            break;

        case REparen:
            // len, ()
            len = ((unsigned*)(prog + 1))[0];
            n = ((unsigned*)(prog + 1))[1];
            pop = prog + 1 + sizeof(unsigned) * 2;
            return _start_chars(r, pop, pop + len);

        case REend:
            return 0;

        case REwordboundary:
        case REnotwordboundary:
            return 0;

        case REdigit:
            r->setbitmax('9');
            for (c = '0'; c <= '9'; c++)
                r->setbit(c);
            return 1;

        case REnotdigit:
            r->setbitmax(0x7F);
            for (c = 0; c <= '0'; c++)
                r->setbit(c);
            for (c = '9' + 1; c <= r->maxc; c++)
                r->setbit(c);
            return 1;

        case REspace:
            r->setbitmax(0x7F);
            for (c = 0; c <= r->maxc; c++)
                if (isspace(c))
                    r->setbit(c);
            return 1;

        case REnotspace:
            r->setbitmax(0x7F);
            for (c = 0; c <= r->maxc; c++)
                if (!isspace(c))
                    r->setbit(c);
            return 1;

        case REword:
            r->setbitmax(0x7F);
            for (c = 0; c <= r->maxc; c++)
                if (isword((char)c))
                    r->setbit(c);
            return 1;

        case REnotword:
            r->setbitmax(0x7F);
            for (c = 0; c <= r->maxc; c++)
                if (!isword((char)c))
                    r->setbit(c);
            return 1;

        case REbackref:
            return 0;

            ///        default:
            ///        L_ASSERTMSG("");
        }
    }
}

//==============================================================================
//
//  Replace
//

// This version of replace() uses:
//  &   replace with the match
//  \n  replace with the nth parenthesized match, n is 1..9
//  \c  replace with char c

char* RegExp::replace(char* format)
{
    RegBuffer buf;
    char* result;
    unsigned c;

    buf.reserve(static_cast<unsigned>((_regex_tcslen(format) + 1) * sizeof(char)));
    for (;; format = _regex_tcsinc(format))
    {
        c = _regex_tcsnextc(format);
        switch (c)
        {
        case 0:
            break;

        case '&':
            buf.write(_input + pmatch[0].begin, (pmatch[0].end - pmatch[0].begin) * sizeof(char));
            continue;

        case '\\':
            format = _regex_tcsinc(format);
            c = _regex_tcsnextc(format);
            if (c >= '1' && c <= '9')
            {
                unsigned i;

                i = c - '0';
                if (i <= re_nsub)
                    buf.write(_input + pmatch[i].begin,
                              (pmatch[i].end - pmatch[i].begin) * sizeof(char));
            }
            else if (!c)
                break;
            else
                buf.writechar(c);
            continue;

        default:
            buf.writechar(c);
            continue;
        }
        break;
    }
    buf.writechar(0);
    result = (char*)buf.data;
    buf.data = NULL;
    return result;
}

// This version of replace uses:
//  $$  $
//  $&  The matched substring.
//  $`  The portion of string that precedes the matched substring.
//  $'  The portion of string that follows the matched substring.
//  $n  The nth capture, where n is a single digit 1-9
//      and $n is not followed by a decimal digit.
//      If n<=m and the nth capture is undefined, use the empty
//      string instead. If n>m, the result is implementation-
//      defined.
//  $nn The nnth capture, where nn is a two-digit decimal
//      number 01-99.
//      If n<=m and the nth capture is undefined, use the empty
//      string instead. If n>m, the result is implementation-
//      defined.
//
//  Any other $ are left as is.
/*
char *RegExp::replace2(char *format)
{
    return replace3(format, input, re_nsub, pmatch);
}
*/
// Static version that doesn't require a RegExp object to be created

char* RegExp::replace3(char* format, char* input, unsigned re_nsub, Match* pmatch)
{
    RegBuffer buf;
    char* result;
    unsigned c;
    unsigned c2;
    int rm_so;
    int rm_eo;
    int i;

    buf.reserve(static_cast<unsigned>((_regex_tcslen(format) + 1) * sizeof(char)));
    for (;; format = _regex_tcsinc(format))
    {
        c = _regex_tcsnextc(format);
    L1:
        if (c == 0)
            break;
        if (c != '$')
        {
            buf.writechar(c);
            continue;
        }
        format = _regex_tcsinc(format);
        c = _regex_tcsnextc(format);
        switch (c)
        {
        case 0:
            buf.writechar('$');
            break;

        case '&':
            rm_so = pmatch[0].begin;
            rm_eo = pmatch[0].end;
            goto Lstring;

        case '`':
            rm_so = 0;
            rm_eo = pmatch[0].begin;
            goto Lstring;

        case '\'':
            rm_so = pmatch[0].end;
            rm_eo = static_cast<int>(_regex_tcslen(input));
            goto Lstring;

        Lstring:
            buf.write(input + rm_so,
                      (rm_eo - rm_so) * sizeof(char));
            continue;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            c2 = format[1];
            if (c2 >= '0' && c2 <= '9')
            {
                i = (c - '0') * 10 + (c2 - '0');
                format = _regex_tcsinc(format);
            }
            else
                i = c - '0';
            if (i == 0)
            {
                buf.writechar('$');
                buf.writechar(c);
                c = c2;
                goto L1;
            }

            if (i <= (int)re_nsub)
            {
                rm_so = pmatch[i].begin;
                rm_eo = pmatch[i].end;
                goto Lstring;
            }
            continue;

        default:
            buf.writechar('$');
            buf.writechar(c);
            continue;
        }
        break;
    }
    buf.writechar(0); // terminate string
    result = (char*)buf.data;
    buf.data = NULL;
    return result;
}

////////////////////////////////////////////////////////
// Return a string that is [input] with [match] replaced by [replacement].

char* RegExp::replace4(char* input, Match* match, char* replacement)
{
    int input_len;
    int replacement_len;
    int result_len;
    char* result;

    input_len = static_cast<int>(_regex_tcslen(input));
    replacement_len = static_cast<int>(_regex_tcslen(replacement));
    result_len = input_len - (match->end - match->begin) + replacement_len;
    result = (char*)malloc((result_len + 1) * sizeof(char));
    memcpy(result, input, match->begin * sizeof(char));
    memcpy(result + match->begin, replacement, replacement_len * sizeof(char));
    memcpy(result + match->begin + replacement_len,
           input + match->end,
           (input_len - match->end) * sizeof(char));
    result[result_len] = 0;
    return result;
}

//------------------------------------------------------------------------------

RegExp::RegBuffer::~RegBuffer()
{
    free(data);
    data = NULL;
}

//------------------------------------------------------------------------------

void RegExp::RegBuffer::reserve(unsigned nbytes)
{
    if (size - offset < nbytes)
    {
        size = (offset + nbytes) * 2;
        data = (unsigned char*)realloc(data, size);
    }
}

//------------------------------------------------------------------------------

void RegExp::RegBuffer::write(const void* data, unsigned nbytes)
{
    reserve(nbytes);
    memcpy(this->data + offset, data, nbytes);
    offset += nbytes;
}

//------------------------------------------------------------------------------

void RegExp::RegBuffer::writeByte(unsigned b)
{
    reserve(1);
    this->data[offset] = (unsigned char)b;
    offset++;
}

//------------------------------------------------------------------------------

void RegExp::RegBuffer::writeword(unsigned w)
{
    reserve(2);
    *(unsigned short*)(this->data + offset) = (unsigned short)w;
    offset += 2;
}

//------------------------------------------------------------------------------

void RegExp::RegBuffer::writechar(char b)
{
    //#if L_PLATFORM == L_PLATFORM_PSP2
    reserve(1);
    char* p = (char*)(this->data + offset);
    *p = b;
    ++offset;
    /*
#else
    char *p;
    size_t len;

    len = _tclen(&b) * sizeof(char);
    reserve(len);
    p = (char *)(this->data + offset);
    _tccpy(p, &b);
    offset += len;
#endif
*/
}

//------------------------------------------------------------------------------

void RegExp::RegBuffer::write4(unsigned w)
{
    reserve(4);
    *(unsigned long*)(this->data + offset) = w;
    offset += 4;
}

//------------------------------------------------------------------------------

void RegExp::RegBuffer::write(RegBuffer* buf)
{
    if (buf)
    {
        reserve(buf->offset);
        memcpy(data + offset, buf->data, buf->offset);
        offset += buf->offset;
    }
}

//------------------------------------------------------------------------------

void RegExp::RegBuffer::fill0(unsigned nbytes)
{
    reserve(nbytes);
    memset(data + offset, 0, nbytes);
    offset += nbytes;
}

//------------------------------------------------------------------------------

void RegExp::RegBuffer::spread(unsigned offset, unsigned nbytes)
{
    reserve(nbytes);
    memmove(data + offset + nbytes, data + offset,
            this->offset - offset);
    this->offset += nbytes;
}

//------------------------------------------------------------------------------

bool RegExp::get_pattern(unsigned n, unsigned /*buf_size*/, char* buf) const
{
    bool success = false;
    const Match* match = pattern(n);

    if (match)
    {
        unsigned len = match->end - match->begin;

        // TODO: check buf-size
        strncpy(buf, _input + match->begin, len);
        buf[len] = '\0';
        success = true;
    }

    return success;
}

//------------------------------------------------------------------------------

bool RegExp::get_pattern(unsigned n, std::string* str) const
{
    bool success = false;
    const Match* match = pattern(n);

    if (match)
    {
        unsigned len = match->end - match->begin;
        char buf[512];

        // TODO: check buf-size
        strncpy(buf, _input + match->begin, len);
        buf[len] = '\0';
        *str = buf;
        success = true;
    }

    return success;
}

#if defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_WIN_UAP__)
#pragma warning(default : 174)
#pragma warning(default : 193)
#pragma warning(default : 810) // conversion from "unsigned int" to "char={char}" may lose significant bits
#pragma warning(pop)
#else
#pragma clang diagnostic pop
#endif
