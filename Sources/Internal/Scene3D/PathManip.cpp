#include "Scene3D/PathManip.h"

namespace DAVA
{
PathManip::PathManip(const char* src)
{
    splitToEntries(src);
}

/*
	PathManip::PathManip(const PathManip& orig) {
	}

	PathManip::~PathManip() {
	}
	 */

void PathManip::splitToEntries(const char* src)
{
    pathEntries.clear();

    String s(src);
    size_t pos = 0;
    bool finish = false;

    while (finish == false)
    {
        size_t n = s.find("/", pos);

        if (n == String::npos)
        {
            finish = true;
            n = s.length();
        }

        size_t len = n - pos;

        if (len)
        {
            pathEntries.push_back(s.substr(pos, len));
        }
        else if (n == 0)
        {
            pathEntries.push_back("/");
        }

        pos = n + 1;

        if (pos >= s.length())
            finish = true;
    }
}

String PathManip::getSuffix()
{
    if (pathEntries.size() == 0)
        return "";

    String& s = pathEntries.back();
    size_t n = s.rfind('.');
    if (n == String::npos)
        return "";

    return s.substr(n, s.length() - n);
}

void PathManip::setSuffix(const String& suf)
{
    if (pathEntries.size() == 0)
    {
        pathEntries.push_back(suf);
        return;
    }

    String& s = pathEntries.back();
    size_t n = s.rfind('.');
    if (n == String::npos)
    {
        s += suf;
        return;
    }

    String news = s.substr(0, n);
    news += suf;
    pathEntries.pop_back();
    pathEntries.push_back(news);
}

String PathManip::GetName()
{
    if (pathEntries.size())
    {
        return pathEntries.back();
    }

    return "";
}

String PathManip::GetPath()
{
    if (pathEntries.size() == 0)
    {
        return "";
    }

    String s;
    size_t n = 0;
    size_t last = pathEntries.size() - 1;

    for (List<String>::iterator i = pathEntries.begin(); n < last; ++i, ++n)
    {
        if ((*i) == "/")
        {
            s += (*i);
        }
        else
        {
            s += (*i) + "/";
        }
    }

    return s;
}

String PathManip::GetString()
{
    if (pathEntries.size() == 0)
    {
        return "";
    }

    String s;
    size_t n = 0;
    size_t last = pathEntries.size() - 1;

    for (List<String>::iterator i = pathEntries.begin(); i != pathEntries.end(); ++i, ++n)
    {
        s += (*i);

        if ((n != last) && ((*i) != "/"))
        {
            s += "/";
        }
    }

    return s;
}

} // ns
