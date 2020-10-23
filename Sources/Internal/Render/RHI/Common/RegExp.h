#if !defined __REGEXP_HPP__
#define __REGEXP_HPP__
//==============================================================================
//
//  Regular expression
//  (adapted from Digital Mars)
//
//==============================================================================
//
//  externals:

//    #include "Debug/DVAssert.h"

    #include <string>

//==============================================================================
//
//  publics:

class
RegExp
{
public:
    RegExp();
    ~RegExp();

    bool compile(const char* pattern, const char* attributes = "");
    bool test(const char* string, int startindex = 0);

    const char* source_text() const;

    // matched patterns ////////////////////////////////////////////////////////

    struct
    Match
    {
        int begin; // index of start of match
        int end; // index past end of match
    };

    unsigned pattern_count() const;
    const Match* pattern(unsigned n) const;
    bool get_pattern(unsigned n, unsigned buf_size, char* buf) const;
    bool get_pattern(unsigned n, std::string* str) const;

private:
    RegExp(const RegExp&); // force no copy
    RegExp& operator=(const RegExp&); // force no assignment

private:
    unsigned re_nsub; // number of parenthesized subexpression matches
    Match* pmatch; // array [re_nsub + 1]

    const char* _input; // the string to search

    // per instance:

    int _is_ref; // !=0 means don't make our own copy of pattern
    char* _pattern; // source text of the regular expression

    char flags[3 + 1]; // source text of the attributes parameter
    // (3 TCHARs max plus terminating 0)
    int _error_count;

    unsigned _attributes;

    enum
    {
        attrGlobal = 1, // has the g attribute
        attrIgnoreCase = 2, // has the i attribute
        attrMultiline = 4, // if treat as multiple lines separated by newlines, or as a single line
        attrDotMatchLF = 8 // if . matches \n
    };

    char* replace(char* format);
    //    TCHAR *replace2(TCHAR *format);
    static char* replace3(char* format, char* input, unsigned re_nsub, Match* pmatch);
    static char* replace4(char* input, Match* match, char* replacement);

private:
    struct Range;
    struct RegBuffer;

    const char* _src; // current source pointer
    const char* _src_start; // starting position for match
    char* _parser_pos; // position of parser in _pattern
    Match _match; // match for the entire regular expression
    // (serves as storage for pmatch[0])

    char* _program;
    RegBuffer* _buf;

    void _print_program(char* prog);
    int _try_match(char* prog, char* progend);
    int _parse_regexp();
    int _parse_piece();
    int _parse_atom();
    int _parse_range();
    int _escape();
    void _error(const char* msg);
    void _optimize();
    int _start_chars(Range* r, char* prog, char* progend);
};

//------------------------------------------------------------------------------

inline unsigned
RegExp::pattern_count() const
{
    return re_nsub + 1;
}

//------------------------------------------------------------------------------

inline const RegExp::Match*
RegExp::pattern(unsigned n) const
{
    //    DVASSERT(n <= re_nsub);

    return (n <= re_nsub) ? pmatch + n : 0;
}

//------------------------------------------------------------------------------

inline const char*
RegExp::source_text() const
{
    return _pattern;
}

//  Escape sequences:
//
//  \nnn starts out a 1, 2 or 3 digit octal sequence,
//  where n is an octal digit. If nnn is larger than
//  0377, then the 3rd digit is not part of the sequence
//  and is not consumed.
//  For maximal portability, use exactly 3 digits.
//
//  \xXX starts out a 1 or 2 digit hex sequence. X
//  is a hex character. If the first character after the \x
//  is not a hex character, the value of the sequence is 'x'
//  and the XX are not consumed.
//  For maximal portability, use exactly 2 digits.
//
//  \uUUUU is a unicode sequence. There are exactly
//  4 hex characters after the \u, if any are not, then
//  the value of the sequence is 'u', and the UUUU are not
//  consumed.
//
//  Character classes:
//
//  [a-b], where a is greater than b, will produce
//  an error.

//==============================================================================
#endif // __REGEXP_HPP__
