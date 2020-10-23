#include "ArchiveExtraction.h"

#include "FileSystem/File.h"
#include "FileSystem/FileSystem.h"

using namespace DAVA;

//We don't have any API for working with ZIP, so we use Python
//https://upload.wikimedia.org/wikipedia/ru/7/78/Trollface.svg
bool RunPythonScript(const String& script)
{
    FileSystem* fs = FileSystem::Instance();
    FilePath scriptFilePath(fs->GetCurrentExecutableDirectory(), "script.py");

    RefPtr<File> scriptFile(File::Create(scriptFilePath, File::CREATE | File::WRITE));
    if (!scriptFile)
        return false;

    scriptFile->WriteString(script);
    //TODO: replace on Reset
    scriptFile = RefPtr<File>();

    int res = ::system(("python.exe " + scriptFilePath.GetAbsolutePathname()).c_str());
    fs->DeleteFile(scriptFilePath);

    return res == 0;
}

bool ExtractFileFromArchive(const String& zipFile, const String& file, const String& outFile)
{
    FileSystem::Instance()->DeleteFile(outFile);
    String outPath = FilePath(outFile).GetDirectory().GetAbsolutePathname();
    String unzippedFile = (FilePath(outPath) + file).GetAbsolutePathname();

    String script = "import zipfile                                      \n"
                    "import os                                           \n"
                    "zf = zipfile.ZipFile('" +
    zipFile + "')             \n"
              "zf.extract('" +
    file + "', '" + outPath + "')       \n"
                              "os.rename('" +
    unzippedFile + "', '" + outFile + "')\n";

    return RunPythonScript(script);
}

bool ExtractAllFromArchive(const String& zipFile, const String& outPath)
{
    FileSystem::Instance()->DeleteDirectory(outPath);
    String script = "import zipfile                         \n"
                    "zf = zipfile.ZipFile('" +
    zipFile + "')\n"
              "zf.extractall('" +
    outPath + "')       \n";

    return RunPythonScript(script);
}