#include "dbg_StatSet.h"
    
    #include "Concurrency/Spinlock.h"
#include "Concurrency/LockGuard.h"

    #include <vector>

//==============================================================================

namespace statset
{
struct
Stat
{
    char full_name[64];
    char short_name[32];
    unsigned parent_id;
    unsigned value;
    unsigned is_permanent : 1;
};

typedef std::vector<Stat> StatArray;

static StatArray _Stat;
static DAVA::Spinlock _StatSync;

//==============================================================================
} // namespace statset

//==============================================================================
//
//  publics:

void StatSet::ResetAll()
{
    using namespace statset;

#if defined(__DAVAENGINE_RENDERSTATS__)

#if defined(RHI_THREADSAFE_STATS)
    DAVA::LockGuard<DAVA::Spinlock> guard(_StatSync);
#endif

    for (StatArray::iterator s = _Stat.begin(), s_end = _Stat.end(); s != s_end; ++s)
    {
        if (!s->is_permanent)
            s->value = 0;
    }

#endif
}

//------------------------------------------------------------------------------

unsigned
StatSet::AddStat(const char* full_name, const char* short_name, unsigned parent_id)
{
    using namespace statset;

    unsigned id = DAVA::InvalidIndex;

#if defined(__DAVAENGINE_RENDERSTATS__)

#if defined(RHI_THREADSAFE_STATS)
    DAVA::LockGuard<DAVA::Spinlock> guard(_StatSync);
#endif

    _Stat.resize(_Stat.size() + 1);

    Stat& stat = _Stat.back();

    strncpy(stat.full_name, full_name, sizeof(stat.full_name));
    strncpy(stat.short_name, short_name, sizeof(stat.short_name));
    stat.parent_id = parent_id;
    stat.value = 0;
    stat.is_permanent = false;

    id = static_cast<unsigned>(_Stat.size()) - 1;

#endif

    return id;
}

//------------------------------------------------------------------------------

unsigned
StatSet::AddPermanentStat(const char* full_name, const char* short_name, unsigned parent_id)
{
    using namespace statset;

    unsigned id = DAVA::InvalidIndex;

#if defined(__DAVAENGINE_RENDERSTATS__)

#if defined(RHI_THREADSAFE_STATS)
    DAVA::LockGuard<DAVA::Spinlock> guard(_StatSync);
#endif

    _Stat.resize(_Stat.size() + 1);

    Stat& stat = _Stat.back();

    strncpy(stat.full_name, full_name, sizeof(stat.full_name));
    strncpy(stat.short_name, short_name, sizeof(stat.short_name));
    stat.parent_id = parent_id;
    stat.value = 0;
    stat.is_permanent = true;

    id = static_cast<unsigned>(_Stat.size()) - 1;

#endif

    return id;
}

//------------------------------------------------------------------------------

void StatSet::SetStat(unsigned id, unsigned value)
{
    using namespace statset;

#if defined(__DAVAENGINE_RENDERSTATS__)

#if defined(RHI_THREADSAFE_STATS)
    DAVA::LockGuard<DAVA::Spinlock> guard(_StatSync);
#endif

    if (id < _Stat.size())
        _Stat[id].value = value;

#endif
}

//------------------------------------------------------------------------------

void StatSet::IncStat(unsigned id, unsigned delta)
{
    using namespace statset;

#if defined(__DAVAENGINE_RENDERSTATS__)

#if defined(RHI_THREADSAFE_STATS)
    DAVA::LockGuard<DAVA::Spinlock> guard(_StatSync);
#endif

    if (id < _Stat.size())
        _Stat[id].value += delta;
#endif
}

//------------------------------------------------------------------------------

void StatSet::DecStat(unsigned id, unsigned delta)
{
    using namespace statset;

#if defined(__DAVAENGINE_RENDERSTATS__)

#if defined(RHI_THREADSAFE_STATS)
    DAVA::LockGuard<DAVA::Spinlock> guard(_StatSync);
#endif

    if (id < _Stat.size())
    {
        unsigned val = _Stat[id].value;
        _Stat[id].value = (delta >= val) ? val - delta : 0;
    }
#endif
}

//------------------------------------------------------------------------------

unsigned
StatSet::StatValue(unsigned id)
{
    using namespace statset;

    unsigned val = 0;

#if defined(__DAVAENGINE_RENDERSTATS__)

#if defined(RHI_THREADSAFE_STATS)
    DAVA::LockGuard<DAVA::Spinlock> guard(_StatSync);
#endif

    val = (id < _Stat.size()) ? _Stat[id].value : 0;
#endif

    return val;
}

//------------------------------------------------------------------------------

unsigned
StatSet::StatID(const char* name)
{
    using namespace statset;

    unsigned id = DAVA::InvalidIndex;

#if defined(__DAVAENGINE_RENDERSTATS__)

#if defined(RHI_THREADSAFE_STATS)
    DAVA::LockGuard<DAVA::Spinlock> guard(_StatSync);
#endif

    for (StatArray::const_iterator s = _Stat.begin(), s_end = _Stat.end(); s != s_end; ++s)
    {
        if (strcmp(name, s->full_name) == 0)
        {
            id = static_cast<unsigned>(s - _Stat.begin());
            break;
        }
    }
#endif

    return id;
}

//------------------------------------------------------------------------------

const char*
StatSet::StatFullName(unsigned id)
{
    using namespace statset;

    const char* name = "<invalid id>";

#if defined(__DAVAENGINE_RENDERSTATS__)

#if defined(RHI_THREADSAFE_STATS)
    DAVA::LockGuard<DAVA::Spinlock> guard(_StatSync);
#endif

    name = (id < _Stat.size()) ? _Stat[id].full_name : "<invalid id>";
#endif
    return name;
}

//------------------------------------------------------------------------------

void StatSet::DumpStats()
{
    using namespace statset;

    /*
    unsigned max_len = 0;

    for( StatArray::Iterator s=_Stat.begin(),s_end=_Stat.end(); s!=s_end; ++s )
    {
        if( s->full_name.length() > max_len )
            max_len = s->full_name.length();
    }
    
    for( StatArray::Iterator s=_Stat.begin(),s_end=_Stat.end(); s!=s_end; ++s )
    {
        char    name[64];

        strcpy( name, s->full_name.c_str() );
        memset( name+s->full_name.length(), ' ', max_len-s->full_name.length() );
        name[max_len] = '\0';

        Trace( "%s = %u\n", name, s->value );
    }
*/
}
