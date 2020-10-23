#include "Classes/Library/Private/LibraryData.h"

const char* LibraryData::selectedPathProperty = "SelectedPath";

const DAVA::FilePath& LibraryData::GetSelectedPath() const
{
    return selectedPath;
}
