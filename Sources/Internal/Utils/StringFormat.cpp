#include <cstdarg>
#include <cstdio>
#include <cmath>

#include "Base/BaseTypes.h"
#include "Debug/DVAssert.h"

namespace DAVA
{
namespace
{
// Special class used in Vsnwprintf for counting neccesary buffer size and preventing buffer overrun
class BufferState
{
public:
    BufferState(char16* buf_, size_t size_)
        : bufEnd(buf_ + size_ - 1)
        , curPtr(buf_)
        , ntotal(0)
    {
    }

    void PlaceChar(char16 c)
    {
        if (curPtr < bufEnd)
        {
            *curPtr++ = c;
        }
        ntotal += 1;
    }

    void PlaceCharN(char16 c, int32 n)
    {
        ntotal += (n > 0 ? n : 0);
        while (n-- > 0 && curPtr < bufEnd)
        {
            *curPtr++ = c;
        }
    }

    void PlaceBuffer(const char16* buf, int32 n, bool reverse = false)
    {
        ntotal += (n > 0 ? n : 0);
        const char16* ptr = buf;
        int32 inc = 1;
        if (reverse)
        {
            ptr = buf + n - 1;
            inc = -1;
        }
        while (n-- > 0 && curPtr < bufEnd)
        {
            *curPtr++ = *ptr;
            ptr += inc;
        }
    }

    void PlaceBuffer(const char8* buf, int32 n, bool reverse = false)
    {
        ntotal += (n > 0 ? n : 0);
        const char8* ptr = buf;
        int32 inc = 1;
        if (reverse)
        {
            ptr = buf + n - 1;
            inc = -1;
        }
        while (n-- > 0 && curPtr < bufEnd)
        {
            *curPtr++ = *ptr;
            ptr += inc;
        }
    }

    void Finish()
    {
        curPtr < bufEnd ? * curPtr = L'\0'
                          :
                          * bufEnd = L'\0';
    }

    size_t Total() const
    {
        return ntotal;
    }

private:
    char16* bufEnd;
    char16* curPtr;
    size_t ntotal;
};

int32 DoDiv(int64& n, int32 base)
{
    unsigned long long unsN = static_cast<unsigned long long>(n);
    unsigned unsBase = static_cast<unsigned>(base);

    int32 result = unsN % unsBase;
    n = unsN / unsBase;
    return result;
}

int32 SkipAtoi(const char16** s)
{
    int32 i = 0;
    while (iswdigit(**s))
    {
        i = i * 10 + *((*s)++) - L'0';
    }
    return i;
}

// Flags used by Vsnwprintf and friends
enum
{
    ZEROPAD = 1, /* pad with zero */
    SIGN = 2, /* unsigned/signed long */
    PLUS = 4, /* show plus */
    SPACE = 8, /* space if plus */
    LEFT = 16, /* left justified */
    SPECIAL = 32, /* 0x */
    LARGE = 64, /* use 'ABCDEF' instead of 'abcdef' */
    LONG_LONG = 128 /* use to convert long long '%lld' */
};

void Number(int64 num, int32 base, int32 size, int32 precision, int32 type, BufferState* state)
{
    const char16* digits = L"0123456789abcdefghijklmnopqrstuvwxyz";
    if (type & LARGE)
        digits = L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (type & LEFT)
        type &= ~ZEROPAD;
    if (base < 2 || base > 36)
        return;

    char16 c = (type & ZEROPAD) ? L'0' : L' ';
    char16 sign = 0;
    if (type & SIGN)
    {
        if (num < 0)
        {
            sign = L'-';
            num = -num;
            --size;
        }
        else if (type & PLUS)
        {
            sign = L'+';
            --size;
        }
        else if (type & SPACE)
        {
            sign = L' ';
            --size;
        }
    }
    if (type & SPECIAL)
    {
        if (base == 16)
        {
            size -= 2;
        }
        else if (base == 8)
        {
            --size;
        }
    }

    char16 tmp[66];
    int32 i = 0;
    if (num == 0)
    {
        tmp[i++] = '0';
    }
    else
    {
        while (num != 0)
        {
            tmp[i++] = digits[DoDiv(num, base)];
        }
    }
    if (i > precision)
        precision = i;

    size -= precision;
    if (!(type & (ZEROPAD + LEFT)))
    {
        state->PlaceCharN(L' ', size);
        size = 0;
    }
    if (sign)
        state->PlaceChar(sign);
    if (type & SPECIAL)
    {
        if (base == 8)
        {
            state->PlaceChar(L'0');
        }
        else if (base == 16)
        {
            state->PlaceChar(L'0');
            state->PlaceChar(digits[33]); // X or x
        }
    }

    if (!(type & LEFT))
    {
        state->PlaceCharN(c, size);
        size = 0;
    }
    state->PlaceCharN(L'0', precision - i);
    state->PlaceBuffer(tmp, i, true);
    state->PlaceCharN(L' ', size);
}

void Numberf(float64 num, int32 base, int32 size, int32 precision, int32 type, BufferState* state)
{
    bool isNegativeValue = false;
    if (num < 0)
    {
        isNegativeValue = true;
        num = -num;
    }

    int32 integerPart = static_cast<int32>(num);
    num -= integerPart;
    for (int32 i = 0; i < precision; ++i)
        num *= 10;
    int32 fracPart = static_cast<int32>(num);
    num -= fracPart;

    if (
// tested on gcc 4.8.1, msvc2013, Apple LLVM version 6.0
#ifdef _MSC_VER
    num >= 0.5f
#else
    num > 0.5f
#endif
    )
    {
        if (precision > 0)
            fracPart++;
        else if (precision == 0)
            integerPart++;
    }

    // Check whether integral part should be incremented due to rounding rules according to precision
    // e.g. 5.99 with precision 1 should become 6.0
    if (fracPart >= pow(10.0, precision))
    {
        integerPart += 1;
        fracPart -= static_cast<int32>(pow(10.0, precision));
    }

    const size_t TEMPBUF_SIZE = 128;
    char16 tempBuf[TEMPBUF_SIZE];
    BufferState tstate(tempBuf, TEMPBUF_SIZE);

    if (isNegativeValue)
        tstate.PlaceChar(L'-');
    Number(integerPart, 10, -1, -1, LEFT, &tstate);
    if (precision > 0)
    {
        tstate.PlaceChar(L'.');
        //Number(fracPart, 10, -1, -1, LEFT, &tstate);
        Number(fracPart, 10, -1, precision, LEFT, &tstate);
    }
    tstate.Finish();
    int32 len = static_cast<int32>(wcslen(tempBuf));
    if (len < size)
    {
        if (!(type & LEFT))
        {
            state->PlaceCharN(L' ', size - len);
            state->PlaceBuffer(tempBuf, len);
        }
        else
        {
            state->PlaceBuffer(tempBuf, len);
            state->PlaceCharN(L' ', size - len);
        }
    }
    else
        state->PlaceBuffer(tempBuf, len);
}

int32 Vsnwprintf(char16* buf, size_t size, const char16* format, va_list& args)
{
    int32 len = 0;
    int32 base = 0;
    int64 num = 0;
    const char8* s = nullptr;
    const char16* sw = nullptr;

    int32 flags = 0; /* flags to Number() */
    int32 field_width = 0; /* width of output field */
    int32 precision = 0; /* min. # of digits for integers; max number of chars for from string */
    int32 qualifier = 0; /* 'h', 'l', 'L', 'w' or 'I' for integer fields */

    if (buf != nullptr && 0 == size)
        return -1;

    char16 dummyStorageOnNullBuffer[1];
    if (nullptr == buf)
    {
        buf = dummyStorageOnNullBuffer;
        size = 1;
    }

    BufferState state(buf, size);
    const char16* fmt = format;
    for (; *fmt; ++fmt)
    {
        if (*fmt != L'%')
        {
            state.PlaceChar(*fmt);
            continue;
        }

        /* process flags */
        flags = 0;
    repeat:
        ++fmt; /* this also skips first '%' */

        switch (*fmt)
        {
        case L'-':
            flags |= LEFT;
            goto repeat;
        case L'+':
            flags |= PLUS;
            goto repeat;
        case L' ':
            flags |= SPACE;
            goto repeat;
        case L'#':
            flags |= SPECIAL;
            goto repeat;
        case L'0':
            flags |= ZEROPAD;
            goto repeat;
        }

        /* get field width */
        field_width = -1;
        if (iswdigit(*fmt))
        {
            field_width = SkipAtoi(&fmt);
        }
        else if (*fmt == L'*')
        {
            ++fmt;
            /* it's the next argument */
            field_width = va_arg(args, int32);
            if (field_width < 0)
            {
                field_width = -field_width;
                flags |= LEFT;
            }
        }
        /* get the precision */
        precision = -1;
        if (*fmt == L'.')
        {
            ++fmt;
            if (iswdigit(*fmt))
            {
                precision = SkipAtoi(&fmt);
            }
            else if (*fmt == L'*')
            {
                ++fmt;
                /* it's the next argument */
                precision = va_arg(args, int32);
            }
            if (precision < 0)
                precision = 0;
        }

        /* get the conversion qualifier */
        qualifier = -1;
        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' || *fmt == 'w')
        {
            qualifier = *fmt;
            ++fmt;
        }
        else if (*fmt == 'I' && *(fmt + 1) == '6' && *(fmt + 2) == '4') // I64
        {
            qualifier = *fmt;
            fmt += 3;
        }

        base = 10; /* default base */
        switch (*fmt)
        {
        case L'c':
            if (!(flags & LEFT))
            {
                state.PlaceCharN(L' ', field_width - 1);
                field_width = 0;
            }
            if (qualifier == 'l')
            {
                char16 c = static_cast<char16>(va_arg(args, int32));
                state.PlaceChar(c);
            }
            else
            {
                char c = static_cast<char>(va_arg(args, int32));
                state.PlaceChar(c);
            }
            state.PlaceCharN(L' ', field_width - 1);
            continue;
        case L'C':
            if (!(flags & LEFT))
            {
                state.PlaceCharN(L' ', field_width - 1);
                field_width = 0;
            }
            if (qualifier == 'l')
            {
                char16 c = static_cast<char16>(va_arg(args, int32));
                state.PlaceChar(c);
            }
            else
            {
                char c = static_cast<char>(va_arg(args, int32));
                state.PlaceChar(c);
            }
            state.PlaceCharN(L' ', field_width - 1);
            continue;
        case L's':
            if (qualifier == 'l')
            {
                /* print unicode string */
                sw = va_arg(args, char16*);
                if (nullptr == sw)
                    sw = L"<NULL>";
                len = static_cast<int32>(wcslen(sw));
                if (precision >= 0 && len > precision)
                    len = precision;
                if (!(flags & LEFT))
                {
                    state.PlaceCharN(L' ', field_width - len);
                    field_width = 0;
                }
                state.PlaceBuffer(sw, len);
                state.PlaceCharN(L' ', field_width - len);
            }
            else
            {
                /* print ascii string */
                s = va_arg(args, char8*);
                if (nullptr == s)
                    s = "<NULL>";
                len = static_cast<int32>(strlen(s));
                if (precision >= 0 && len > precision)
                    len = precision;
                if (!(flags & LEFT))
                {
                    state.PlaceCharN(L' ', field_width - len);
                    field_width = 0;
                }
                state.PlaceBuffer(s, len);
                state.PlaceCharN(L' ', field_width - len);
            }
            continue;
        case L'S':
            if (qualifier == 'l' || qualifier == 'w')
            {
                /* print unicode string */
                sw = va_arg(args, char16*);
                if (nullptr == sw)
                    sw = L"<NULL>";
                len = static_cast<int32>(wcslen(sw));
                if (precision >= 0 && len > precision)
                    len = precision;
                if (!(flags & LEFT))
                {
                    state.PlaceCharN(L' ', field_width - len);
                    field_width = 0;
                }
                state.PlaceBuffer(sw, len);
                state.PlaceCharN(L' ', field_width - len);
            }
            else
            {
                /* print ascii string */
                s = va_arg(args, char8*);
                if (nullptr == s)
                    s = "<NULL>";
                len = static_cast<int32>(strlen(s));
                if (precision >= 0 && len > precision)
                    len = precision;
                if (!(flags & LEFT))
                {
                    state.PlaceCharN(L' ', field_width - len);
                    field_width = 0;
                }
                state.PlaceBuffer(s, len);
                state.PlaceCharN(L' ', field_width - len);
            }
            continue;
        case L'Z': // %Z is not supported
            DVASSERT(0);
            continue;
        case L'p':
            if (field_width == -1)
            {
                field_width = 2 * sizeof(void*);
                flags |= ZEROPAD;
            }
            {
                uintptr_t v = reinterpret_cast<uintptr_t>(va_arg(args, void*));
                Number(v, 16, field_width, precision, flags, &state);
            }
            continue;
        case L'n':
            DVASSERT(0); // %n is not supported
            continue;
        /* integer number formats - set up the flags and "break" */
        case L'o':
            base = 8;
            break;
        case L'b':
            base = 2;
            break;
        case L'X':
            flags |= LARGE;
        case L'x':
            base = 16;
            break;
        case L'd':
        case L'i':
            flags |= SIGN;
        case L'u':
            break;
        case L'l':
            if (*fmt == L'l')
            {
                char16 next = *(fmt + 1);
                if (next == L'd') // %lld
                {
                    flags |= LONG_LONG | SIGN;
                    fmt++;
                }
                else if (next == L'u') // %llu
                {
                    flags |= LONG_LONG;
                    fmt++;
                }
                else if (next == L'X') // %llX
                {
                    base = 16;
                    flags |= LONG_LONG | LARGE;
                    fmt++;
                }
                else if (next == L'x') // %llx
                {
                    base = 16;
                    flags |= LONG_LONG;
                    fmt++;
                }
            }
            break;
        case L'f':
        {
            base = -1;
            qualifier = 'f';
            flags |= SIGN;
        }
        break;
        default:
            if (*fmt != L'%')
                state.PlaceChar(L'%');
            if (*fmt)
                state.PlaceChar(*fmt);
            else
                --fmt;
            continue;
        }

        if (qualifier == 'f')
        {
            float64 floatValue = va_arg(args, float64);
            if (precision < 0)
                precision = 6;
            Numberf(floatValue, base, field_width, precision, flags, &state);
        }
        else
        {
            if (qualifier == 'I')
            {
                num = va_arg(args, uint64);
            }
            else if (qualifier == 'l')
            {
                if (flags & LONG_LONG)
                    num = flags & SIGN ? va_arg(args, int64) : va_arg(args, uint64);
                else
                    num = va_arg(args, unsigned long);
            }
            else
            {
                if (flags & SIGN)
                    num = va_arg(args, int32);
                else
                    num = va_arg(args, uint32);
            }
            Number(num, base, field_width, precision, flags, &state);
        }
    }
    state.Finish();
    return static_cast<int32>(state.Total());
}

int FormattedLengthV(const char8* format, va_list args)
{
    DVASSERT(format != nullptr);
#if defined(__DAVAENGINE_WINDOWS__)
    int result = _vscprintf(format, args);
    DVASSERT(result >= 0);
    return result;
#else
    // To obtain neccesary buffer length pass null buffer and 0 as buffer length
    int result = vsnprintf(nullptr, 0, format, args);
    DVASSERT(result >= 0);
    return result;
#endif
}

int FormattedLengthV(const char16* format, va_list& args)
{
    DVASSERT(format != nullptr);
    // To obtain neccesary buffer length pass null buffer and 0 as buffer length
    int result = Vsnwprintf(nullptr, 0, format, args);
    DVASSERT(result >= 0);
    return result;
}

} // unnamed namespace

//////////////////////////////////////////////////////////////////////////

//! Formatting functions (use printf-like syntax (%ls for WideString))
String FormatVL(const char8* format, va_list& args)
{
    String result;
    int32 length = 0;
    {
        va_list xargs;
        va_copy(xargs, args); // args cannot be used twice without copying
        length = FormattedLengthV(format, xargs);
        va_end(xargs);
    }
    if (length >= 0)
    {
        result.resize(length + 1);
        vsnprintf(&*result.begin(), length + 1, format, args);
        result.pop_back();
    }
    return result;
}

WideString FormatVL(const char16* format, va_list& args)
{
    WideString result;
    int32 length = 0;
    {
        va_list xargs;
        va_copy(xargs, args); // args cannot be used twice without copying
        length = FormattedLengthV(format, xargs);
        va_end(xargs);
    }
    if (length >= 0)
    {
        result.resize(length + 1);
        Vsnwprintf(&*result.begin(), length + 1, format, args);
        result.pop_back();
    }
    return result;
}

String Format(const char8* format, ...)
{
    va_list args;
    va_start(args, format);
    String result = FormatVL(format, args);
    va_end(args);
    return result;
}

//  Format(L"") use case with WideString parameter:
//         WideString info( Format(L"%ls", tank->GetName().c_str()) ); // tank->GetName() -> WideString&
//         activeTankInfo->SetText(info);
WideString Format(const char16* format, ...)
{
    va_list args;
    va_start(args, format);
    WideString result = FormatVL(format, args);
    va_end(args);
    return result;
}

} // namespace DAVA
