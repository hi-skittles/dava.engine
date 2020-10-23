#ifndef ARCHIVE_EXTRACTION_H
#define ARCHIVE_EXTRACTION_H

#include "Base/BaseTypes.h"

bool ExtractFileFromArchive(const DAVA::String& zipFile,
                            const DAVA::String& file,
                            const DAVA::String& outFile);

bool ExtractAllFromArchive(const DAVA::String& zipFile, const DAVA::String& outPath);

#endif // ARCHIVE_EXTRACTION_H