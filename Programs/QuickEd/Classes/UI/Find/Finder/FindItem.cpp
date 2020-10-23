#include "UI/Find/Finder/FindItem.h"

FindItem::FindItem()
{
}

FindItem::FindItem(const DAVA::FilePath& file_)
    : file(file_)
{
}

FindItem::~FindItem()
{
}

const DAVA::FilePath& FindItem::GetFile() const
{
    return file;
}

const DAVA::Vector<DAVA::String>& FindItem::GetControlPaths() const
{
    return controlPaths;
}

void FindItem::AddPathToControl(const DAVA::String& path)
{
    controlPaths.push_back(path);
}

bool FindItem::IsValid() const
{
    return !file.IsEmpty();
}

void FindItem::Reset()
{
    file = DAVA::FilePath();
    controlPaths.clear();
}
