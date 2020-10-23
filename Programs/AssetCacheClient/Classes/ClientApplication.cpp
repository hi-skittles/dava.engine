#include "ClientApplication.h"

#include "AddRequest.h"
#include "GetRequest.h"
#include "RemoveRequest.h"
#include "ClearRequest.h"

ClientApplication::ClientApplication()
{
    requests.emplace_back(std::unique_ptr<CacheRequest>(new AddRequest()));
    requests.emplace_back(std::unique_ptr<CacheRequest>(new GetRequest()));
    requests.emplace_back(std::unique_ptr<CacheRequest>(new RemoveRequest()));
    requests.emplace_back(std::unique_ptr<CacheRequest>(new ClearRequest()));
}

ClientApplication::~ClientApplication()
{
    activeRequest = nullptr;
}

bool ClientApplication::ParseCommandLine(const DAVA::Vector<DAVA::String>& cmdLine)
{
    if (cmdLine.size() > 1)
    {
        for (auto& r : requests)
        {
            auto commandLineIsOk = r->options.Parse(cmdLine);
            if (commandLineIsOk)
            {
                activeRequest = r.get();
                exitCode = activeRequest->CheckOptions();
                break;
            }
        }

        if (exitCode != DAVA::AssetCache::Error::NO_ERRORS)
        {
            PrintUsage();
            return false;
        }
        return true;
    }
    else
    {
        PrintUsage();
        exitCode = DAVA::AssetCache::Error::WRONG_COMMAND_LINE;
    }
    return false;
}

void ClientApplication::PrintUsage() const
{
    printf("\nUsage: AssetCacheClient <command>\n");
    printf("\n Commands: ");

    auto count = requests.size();
    for (auto& r : requests)
    {
        printf("%s", r->options.GetCommand().c_str());
        if (count != 1)
        {
            printf(", ");
        }
        --count;
    }

    printf("\n\n");
    for (auto& r : requests)
    {
        printf("%s\n", r->options.GetUsageString().c_str());
        printf("\n");
    }
}

void ClientApplication::Process()
{
    DVASSERT(activeRequest != nullptr);

    exitCode = activeRequest->Process(cacheClient);
}
