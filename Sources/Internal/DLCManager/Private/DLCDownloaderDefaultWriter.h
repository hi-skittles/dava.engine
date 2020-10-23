#pragma once

#include "Base/BaseTypes.h"
#include "Base/RefPtr.h"
#include "DLCManager/DLCDownloader.h"

namespace DAVA
{
class File;

struct DLCDownloaderDefaultWriter :
DLCDownloader::IWriter
{
    explicit DLCDownloaderDefaultWriter(const String& outputFile);
    ~DLCDownloaderDefaultWriter();

    void MoveToEndOfFile() const;
    uint64 Save(const void* ptr, uint64 size) override;
    uint64 GetSeekPos() override;
    bool Truncate() override;
    bool Close() override;
    bool IsClosed() const override;

private:
    RefPtr<File> f;
};
}