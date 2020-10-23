#pragma once

#include "Base/BaseTypes.h"
#include "FileSystem/FilePath.h"

class ResaveUtility
{
public:
    ResaveUtility();

    void Resave();

private:
    DAVA::List<DAVA::FilePath> filesToResave;
};