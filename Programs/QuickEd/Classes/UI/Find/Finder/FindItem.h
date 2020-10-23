#pragma once

#include <Base/BaseTypes.h>
#include <FileSystem/FilePath.h>

class FindItem
{
public:
    FindItem();
    FindItem(const DAVA::FilePath& file);
    ~FindItem();

    const DAVA::FilePath& GetFile() const;

    const DAVA::Vector<DAVA::String>& GetControlPaths() const;
    void AddPathToControl(const DAVA::String& control);

    bool IsValid() const;
    void Reset();

private:
    DAVA::FilePath file;
    DAVA::Vector<DAVA::String> controlPaths;
};
