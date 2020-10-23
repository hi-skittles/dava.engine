#ifndef _PATHMANIP_H
#define _PATHMANIP_H

#include "Base/BaseTypes.h"

namespace DAVA
{
// TODO
class PathManip
{
public:
    PathManip(const char* src);
    //PathManip(const PathManip& orig);
    //virtual ~PathManip();
    /**
		 * Returns path as string
		 */
    String GetString();

    /// returns path to last entry
    String GetPath();

    /// returns last entry with suffix
    String GetName();

    /// returns suffix, e.g. ".png" or empty string if none
    String getSuffix();

    void setSuffix(const String& s);

private:
    List<String> pathEntries;

    void splitToEntries(const char* src);
};

} // ns

#endif /* _PATHMANIP_H */
