#pragma once

#include "Base/BaseTypes.h"
#include "Debug/DVAssert.h"
#include "Utils/StringFormat.h"

#include <fstream>
#include <iostream>
#include <iomanip>

namespace mitsuba
{
extern const DAVA::String kBeginTag;
extern const DAVA::String kBeginCloseTag;
extern const DAVA::String kEndTag;
extern const DAVA::String kSelfEndTag;
extern const DAVA::String kSpace;
extern const DAVA::String kEqual;
extern const DAVA::String kQuote;
extern const DAVA::String kIdent;
extern const DAVA::String kName;
extern const DAVA::String kValue;
extern const DAVA::String kType;
extern const DAVA::String kInteger;
extern const DAVA::String kString;
extern const DAVA::String kFloat;
extern const DAVA::String kId;
extern const DAVA::String kBumpmap;
extern const DAVA::String kDiffuse;
extern const DAVA::String kDielectric;
extern const DAVA::String kRoughDielectric;
extern const DAVA::String kRoughConductor;

extern DAVA::uint32 outputIdentation;
extern std::ostream* currentOutput;

template <class T>
DAVA::String ToString(const T& t);
template <>
inline DAVA::String ToString<DAVA::String>(const DAVA::String& t)
{
    return t;
}
#define DECL_FOMAT(TYPE, FMT) template <> inline DAVA::String ToString<TYPE>(const TYPE& t) { return DAVA::Format(FMT, t); }
DECL_FOMAT(DAVA::int32, "%d")
DECL_FOMAT(DAVA::uint32, "%u")
DECL_FOMAT(DAVA::int64, "%ld")
DECL_FOMAT(DAVA::uint64, "%lu")
DECL_FOMAT(DAVA::float32, "%f")
DECL_FOMAT(DAVA::float64, "%f")
#undef DECL_FOMAT

inline std::ostream& ident(std::ostream& out_)
{
    for (DAVA::uint32 i = 0; i < outputIdentation; ++i)
        out_ << kIdent;
    return out_;
}

struct scope
{
    template <class... T>
    scope(const DAVA::String& name_, T&&... args)
        : name(name_)
    {
        (*currentOutput) << ident << kBeginTag << name;
        DAVA::Vector<DAVA::String> properties = { ToString(args)... };
        if (properties.size() > 0)
        {
            DVASSERT(properties.size() % 2 == 0);
            for (auto i = properties.begin(), e = properties.end(); i != e; i += 2)
                (*currentOutput) << kSpace << (*i) << kEqual << kQuote << *(i + 1) << kQuote;
        }
        (*currentOutput) << kEndTag << std::endl;
        ++outputIdentation;
    }

    ~scope()
    {
        --outputIdentation;
        (*currentOutput) << ident << kBeginCloseTag << name << kEndTag << std::endl;
    }

    DAVA::String name;
};

template <class... T>
void tag(const DAVA::String& name, T&&... args)
{
    (*currentOutput) << ident << kBeginTag << name;
    DAVA::Vector<DAVA::String> properties = { ToString(args)... };
    if (properties.size() > 0)
    {
        DVASSERT(properties.size() % 2 == 0);
        for (auto i = properties.begin(), e = properties.end(); i != e; i += 2)
            (*currentOutput) << kSpace << (*i) << kEqual << kQuote << *(i + 1) << kQuote;
    }
    (*currentOutput) << kSelfEndTag << std::endl;
}
}
