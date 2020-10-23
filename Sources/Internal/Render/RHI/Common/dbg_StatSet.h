#if !defined __STATSET_HPP__
#define __STATSET_HPP__
//==============================================================================
//
//  Global Set of (usually performance-related) per-frame stats
//
//==============================================================================
//
//  externals:

    #include "../rhi_Type.h"

//==============================================================================
//
//  publics:

class
StatSet
{
public:
    static unsigned AddStat(const char* full_name, const char* short_name, unsigned parent_id = DAVA::InvalidIndex);
    static unsigned AddPermanentStat(const char* full_name, const char* short_name, unsigned parent_id = DAVA::InvalidIndex);

    static void ResetAll();

    static void SetStat(unsigned id, unsigned value);
    static void IncStat(unsigned id, unsigned delta);
    static void DecStat(unsigned id, unsigned delta);

    static unsigned StatValue(unsigned id);

    static unsigned StatID(const char* name);
    static const char* StatFullName(unsigned id);
    static void DumpStats();
};

//==============================================================================
#endif // __STATSET_HPP__
