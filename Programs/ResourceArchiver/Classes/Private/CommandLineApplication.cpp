#include "CommandLineApplication.h"
#include <Logger/Logger.h>
#include <Engine/Engine.h>
#include <FileSystem/FileSystem.h>
#include <FileSystem/FileList.h>

#include <sqlite_modern_cpp.h>

static bool DoSelftest(DAVA::String pathToCurrentBinary);

CommandLineApplication::CommandLineApplication(DAVA::String name)
    : appName(name)
    , helpOption("help")
{
}

void CommandLineApplication::AddTool(std::unique_ptr<CommandLineTool> tool)
{
    tools.emplace_back(std::move(tool));
}

int CommandLineApplication::Process(const DAVA::Vector<DAVA::String>& cmdline)
{
    if (cmdline.size() > 1)
    {
        const DAVA::String command = cmdline[1];
        if (command == "--selftest")
        {
            DAVA::String pathToCurrentBinary = cmdline[0];
            if (!DoSelftest(pathToCurrentBinary))
            {
                return EXIT_FAILURE;
            }
            return EXIT_SUCCESS;
        }
    }

    if (helpOption.Parse(cmdline) == true)
    {
        PrintUsage();
        return EXIT_SUCCESS;
    }

    for (auto& tool : tools)
    {
        const bool parsed = tool->ParseOptions(cmdline);
        if (parsed)
        {
            return tool->Process();
        }
    }

    PrintUsage();
    return EXIT_FAILURE;
}

void CommandLineApplication::PrintUsage()
{
    std::stringstream ss;
    ss << "Usage: " << appName << " <command>" << std::endl;
    ss << " Commands: ";

    for (const auto& tool : tools)
    {
        ss << tool->GetToolKey() << ", ";
    }
    ss << helpOption.GetCommand() << std::endl
       << std::endl;

    for (const auto& tool : tools)
    {
        ss << tool->GetUsageString();
        ss << std::endl;
    }

    DAVA::Logger::Info("%s", ss.str().c_str());
}

static void GenerateSubfoldersWithFiles(DAVA::String sourceDir);
static void GenerateMetaDB(DAVA::String sourceDir, DAVA::String metaDB);
static void GenerateSuperpack(DAVA::String binary, DAVA::String sourceDir, DAVA::String metaDB, DAVA::String superpack);
static void UnpackSuperpak(DAVA::String binary, DAVA::String pack, DAVA::String unpackDir);
static void CompareFoldersByContent(DAVA::String destDir, DAVA::String srcDir);
static void DeleteAll(DAVA::String destDir, DAVA::String srcDir, DAVA::String metaDB, DAVA::String superpack);

static bool DoSelftest(DAVA::String pathToCurrentBinary)
{
    try
    {
        // 1 generate folder with subfolders and files
        GenerateSubfoldersWithFiles("res_archiver_selftest_src/");

        // 2 generate sqlite3 DB from generated files
        GenerateMetaDB("res_archiver_selftest_src/", "metaDB.db");

        // 3 generate superpack.dvpk with this tool
        GenerateSuperpack(pathToCurrentBinary, "res_archiver_selftest_src/", "metaDB.db", "pack.dvpk");

        // 4 unpack content of generated duperpack.dvpk with this tool
        UnpackSuperpak(pathToCurrentBinary, "pack.dvpk", "res_arhiver_selftest_unpack/");

        // 5 compare all unpacked files with source files
        CompareFoldersByContent("res_arhiver_selftest_unpack/", "res_archiver_selftest_src/");
    }
    catch (std::exception& ex)
    {
        DAVA::Logger::Error("error selftest failed: %s", ex.what());
        DeleteAll("res_arhiver_selftest_unpack/", "res_archiver_selftest_src/", "metaDB.db", "pack.dvpk");
        return false;
    }
    DeleteAll("res_arhiver_selftest_unpack/", "res_archiver_selftest_src/", "metaDB.db", "pack.dvpk");
    return true;
}

static DAVA::Vector<std::tuple<DAVA::String, DAVA::String, DAVA::String, int>> files = {
    { "dir1", "file_name1.txt", "content_of_the_file", 0 },
    { "dir2", "file_name2.txt", "content", 1 },
    { "dir2", "file_name3.txt", "1", 1 },
    { "dir3", "file_name4.txt", "2", 2 },
    { "dir3", "file_name5.txt", "3", 2 }
};

static void GenerateSubfoldersWithFiles(DAVA::String sourceDir)
{
    using namespace DAVA;
    FileSystem* fs = GetEngineContext()->fileSystem;

    const FilePath cwd = fs->GetCurrentWorkingDirectory();
    const FilePath srcDir = cwd + sourceDir;

    for (auto& fileInfo : files)
    {
        const FilePath dir = srcDir + "/" + std::get<0>(fileInfo);
        fs->CreateDirectory(dir, true);

        const FilePath fileName = dir + "/" + std::get<1>(fileInfo);

        ScopedPtr<File> f(File::Create(fileName, File::WRITE | File::CREATE));
        if (!f)
        {
            DAVA_THROW(Exception, "can't create test file");
        }
        String fileContent = std::get<2>(fileInfo);
        uint32 size = f->Write(fileContent.data(), static_cast<uint32>(fileContent.size()));
        if (size != fileContent.size())
        {
            DAVA_THROW(Exception, "can't write file");
        }
    }
}

static void GenerateMetaDB(DAVA::String sourceDir, DAVA::String metaDB)
{
    DAVA::FileSystem* fs = DAVA::GetEngineContext()->fileSystem;
    fs->DeleteFile(metaDB);

    sqlite::database db(metaDB);

    db << "CREATE TABLE IF NOT EXISTS files (path TEXT PRIMARY KEY, pack_index INTEGER NOT NULL);";
    db << "CREATE TABLE IF NOT EXISTS packs (\"index\" INTEGER PRIMARY KEY, name TEXT UNIQUE, dependency TEXT NOT NULL);";

    for (auto& fileInfo : files)
    {
        const DAVA::String path = std::get<0>(fileInfo) + "/" + std::get<1>(fileInfo);
        int packIndex = std::get<3>(fileInfo);
        db << "INSERT INTO files (path, pack_index) VALUES (?, ?);"
           << path
           << packIndex;
    }

    db << "INSERT INTO packs (\"index\", name, dependency) VALUES (?, ?, ?);"
       << 0
       << "pack0"
       << "";

    db << "INSERT INTO packs (\"index\", name, dependency) VALUES (?, ?, ?);"
       << 1
       << "pack1"
       << "0";

    db << "INSERT INTO packs (\"index\", name, dependency) VALUES (?, ?, ?);"
       << 2
       << "pack2"
       << "1";
}

static void GenerateSuperpack(DAVA::String binary, DAVA::String sourceDir, DAVA::String metaDB, DAVA::String superpack)
{
    DAVA::String command(binary + " pack -metadb " + metaDB + " -basedir " + sourceDir + " " + superpack);
    const int result = system(command.c_str());
    if (result != EXIT_SUCCESS)
    {
        DAVA_THROW(DAVA::Exception, "command failed: " + command);
    }
}

static void UnpackSuperpak(DAVA::String binary, DAVA::String packfile, DAVA::String unpackDir)
{
    using namespace DAVA;
    String command(binary + " unpack " + packfile + " " + unpackDir);
    const int result = system(command.c_str());
    if (result != EXIT_SUCCESS)
    {
        DAVA_THROW(DAVA::Exception, "command failed: " + command);
    }
}

static void CompareFoldersByContent(DAVA::String destDir, DAVA::String srcDir)
{
    using namespace DAVA;
    ScopedPtr<FileList> filesDst(new FileList(destDir));
    ScopedPtr<FileList> filesSrc(new FileList(srcDir));

    if (filesDst->GetCount() != filesSrc->GetCount())
    {
        DAVA_THROW(DAVA::Exception, "file lists mismatch");
    }

    if (filesDst->GetDirectoryCount() != filesSrc->GetDirectoryCount())
    {
        DAVA_THROW(DAVA::Exception, "file lists mismatch (directory count)");
    }

    if (filesDst->GetFileCount() != filesSrc->GetFileCount())
    {
        DAVA_THROW(DAVA::Exception, "file lists mismatch (file count)");
    }

    FileSystem* fs = GetEngineContext()->fileSystem;

    for (uint32 fileIndex = 0; fileIndex < filesDst->GetCount(); ++fileIndex)
    {
        if (!filesDst->IsDirectory(fileIndex))
        {
            String f1 = filesDst->GetFilename(fileIndex);
            String f2 = filesSrc->GetFilename(fileIndex);
            if (f1 != f2)
            {
                DAVA_THROW(DAVA::Exception, "file names mismatch");
            }
            Vector<uint8> f1content;
            fs->ReadFileContents(FilePath(destDir + "/" + f1), f1content);

            Vector<uint8> f2content;
            fs->ReadFileContents(FilePath(destDir + "/" + f2), f2content);

            if (f1content != f2content)
            {
                DAVA_THROW(Exception, "file content mismatch");
            }
        }
    }

    for (uint32 dirIndex = 0; dirIndex < filesDst->GetCount(); ++dirIndex)
    {
        if (filesDst->IsDirectory(dirIndex) && !filesDst->IsNavigationDirectory(dirIndex))
        {
            FilePath d1 = filesDst->GetPathname(dirIndex);
            FilePath d2 = filesSrc->GetPathname(dirIndex);
            String dirName1 = d1.GetLastDirectoryName();
            String dirName2 = d2.GetLastDirectoryName();
            if (dirName1 != dirName2)
            {
                DAVA_THROW(DAVA::Exception, "direcotry names mismatch");
            }
            // recursively call to check files inside directory
            CompareFoldersByContent(destDir + dirName1 + "/", srcDir + dirName2 + "/");
        }
    }
}

static void DeleteAll(DAVA::String destDir, DAVA::String srcDir, DAVA::String metaDB, DAVA::String superpack)
{
    using namespace DAVA;

    FileSystem* fs = GetEngineContext()->fileSystem;

    fs->DeleteDirectory(destDir);
    fs->DeleteDirectory(srcDir);
    fs->DeleteFile(metaDB);
    fs->DeleteFile(superpack);
}
