#include <DocDirSetup/DocDirSetup.h>

#include <Engine/Engine.h>
#include <DLC/Patcher/PatchFile.h>
#include <FileSystem/FileSystem.h>
#include <FileSystem/VariantType.h>
#include <CommandLine/ProgramOptions.h>
#include <Debug/DVAssertDefaultHandlers.h>

using namespace DAVA;

void PrintError(DAVA::PatchFileReader::PatchError error)
{
    switch (error)
    {
    case DAVA::PatchFileReader::ERROR_NO:
        break;
    case DAVA::PatchFileReader::ERROR_EMPTY_PATCH:
        printf("ERROR_EMPTY_PATCH\n");
        break;
    case DAVA::PatchFileReader::ERROR_ORIG_READ:
        printf("ERROR_ORIG_PATH\n");
        break;
    case DAVA::PatchFileReader::ERROR_ORIG_FILE_CRC:
        printf("ERROR_ORIG_FILE_CRC\n");
        break;
    case DAVA::PatchFileReader::ERROR_ORIG_BUFFER_CRC:
        printf("ERROR_ORIG_BUFFER_CRC\n");
        break;
    case DAVA::PatchFileReader::ERROR_NEW_WRITE:
        printf("ERROR_NEW_PATH\n");
        break;
    case DAVA::PatchFileReader::ERROR_NEW_CRC:
        printf("ERROR_NEW_CRC\n");
        break;
    case DAVA::PatchFileReader::ERROR_CANT_READ:
        printf("ERROR_CANT_READ\n");
        break;
    case DAVA::PatchFileReader::ERROR_CORRUPTED:
        printf("ERROR_CORRUPTED\n");
        break;
    case DAVA::PatchFileReader::ERROR_UNKNOWN:
    default:
        printf("ERROR_UNKNOWN\n");
        break;
    }
}

int DoPatch(DAVA::PatchFileReader* reader, const DAVA::FilePath& origBase, const DAVA::FilePath& origPath, const DAVA::FilePath& newBase, const DAVA::FilePath& newPath)
{
    int ret = 0;

    if (!reader->Apply(origBase, origPath, newBase, newPath))
    {
        PrintError(reader->GetError());
        ret = 1;
    }

    return ret;
}

int Process(Engine& e)
{
    int ret = 0;

    DAVA::ProgramOptions writeOptions("write");
    DAVA::ProgramOptions listOptions("list");
    DAVA::ProgramOptions applyOptions("apply");
    DAVA::ProgramOptions applyAllOptions("apply-all");

    writeOptions.AddOption("-a", DAVA::VariantType(false), "Append patch to existing file.");
    writeOptions.AddOption("-nc", DAVA::VariantType(false), "Generate uncompressed patch.");
    writeOptions.AddOption("-v", DAVA::VariantType(false), "Verbose output.");
    writeOptions.AddOption("-bo", DAVA::VariantType(DAVA::String("")), "Original file base dir.");
    writeOptions.AddOption("-bn", DAVA::VariantType(DAVA::String("")), "New file base dir.");
    writeOptions.AddArgument("OriginalFile");
    writeOptions.AddArgument("NewFile");
    writeOptions.AddArgument("PatchFile");

    listOptions.AddArgument("PatchFile");
    listOptions.AddOption("-v", DAVA::VariantType(false), "Verbose output.");

    applyOptions.AddOption("-i", DAVA::VariantType(DAVA::String("")), "Input original file.");
    applyOptions.AddOption("-o", DAVA::VariantType(DAVA::String("")), "Output file or directory.");
    applyOptions.AddOption("-bo", DAVA::VariantType(DAVA::String("")), "Original file base dir.");
    applyOptions.AddOption("-bn", DAVA::VariantType(DAVA::String("")), "New file base dir.");
    applyOptions.AddOption("-v", DAVA::VariantType(false), "Verbose output.");
    applyOptions.AddArgument("PatchIndex");
    applyOptions.AddArgument("PatchFile");

    applyAllOptions.AddArgument("PatchFile");
    applyAllOptions.AddOption("-bo", DAVA::VariantType(DAVA::String("")), "Original file base dir.");
    applyAllOptions.AddOption("-bn", DAVA::VariantType(DAVA::String("")), "New file base dir.");
    applyAllOptions.AddOption("-t", DAVA::VariantType(false), "Truncate patch file, when applying it.");
    applyAllOptions.AddOption("-v", DAVA::VariantType(false), "Verbose output.");

    FileSystem* fileSystem = e.GetContext()->fileSystem;

    DocumentsDirectorySetup::SetApplicationDocDirectory(fileSystem, "ResourcePatcher");

    const Vector<String>& cmdLine = e.GetCommandLine();

    bool paramsOk = false;
    if (cmdLine.size() > 1)
    {
        //DAVA::String command = DAVA::CommandLineParser::Instance()->GetCommand(0);

        const String& command = cmdLine[1];
        if (command == writeOptions.GetCommand())
        {
            paramsOk = writeOptions.Parse(cmdLine);
            if (paramsOk)
            {
                DAVA::PatchFileWriter::WriterMode writeMode = DAVA::PatchFileWriter::WRITE;
                BSType bsType = BS_ZLIB;

                if (writeOptions.GetOption("-a").AsBool())
                {
                    writeMode = DAVA::PatchFileWriter::APPEND;
                }

                if (writeOptions.GetOption("-nc").AsBool())
                {
                    bsType = BS_PLAIN;
                }

                DAVA::FilePath origPath = writeOptions.GetArgument("OriginalFile");
                DAVA::FilePath newPath = writeOptions.GetArgument("NewFile");
                DAVA::FilePath patchPath = writeOptions.GetArgument("PatchFile");

                DAVA::FilePath origBasePath = writeOptions.GetOption("-bo").AsString();
                DAVA::FilePath newBasePath = writeOptions.GetOption("-bn").AsString();
                bool verbose = writeOptions.GetOption("-v").AsBool();

                if (!origBasePath.IsEmpty() && !origBasePath.IsDirectoryPathname())
                {
                    printf("Bad original base dir\n");
                    ret = 1;
                }

                if (!newBasePath.IsEmpty() && !newBasePath.IsDirectoryPathname())
                {
                    printf("Bad new base dir\n");
                    ret = 1;
                }

                if (0 == ret)
                {
                    DAVA::PatchFileWriter patchWriter(patchPath, writeMode, bsType, verbose);
                    if (!patchWriter.Write(origBasePath, origPath, newBasePath, newPath))
                    {
                        printf("Error, while creating patch [%s] -> [%s].\n", origPath.GetRelativePathname().c_str(), newPath.GetRelativePathname().c_str());
                        ret = 1;
                    }
                }
            }
        }
        else if (command == listOptions.GetCommand())
        {
            paramsOk = listOptions.Parse(cmdLine);
            if (paramsOk)
            {
                DAVA::uint32 index = 0;
                DAVA::FilePath patchPath = listOptions.GetArgument("PatchFile");
                bool verbose = listOptions.GetOption("-v").AsBool();

                if (!patchPath.Exists())
                {
                    printf("No such file %s\n", patchPath.GetAbsolutePathname().c_str());
                    ret = 1;
                }

                if (0 == ret)
                {
                    DAVA::PatchFileReader patchReader(patchPath);
                    patchReader.ReadFirst();

                    const DAVA::PatchInfo* patchInfo = patchReader.GetCurInfo();
                    while (NULL != patchInfo)
                    {
                        DAVA::String origStr = patchInfo->origPath;
                        DAVA::String newStr = patchInfo->newPath;

                        if (origStr.empty())
                            origStr = "[]";
                        if (newStr.empty())
                            newStr = "[]";

                        printf("  %4u: %s --> %s\n", index, origStr.c_str(), newStr.c_str());
                        if (verbose)
                        {
                            printf("     OrigSize: %u byte; OrigCRC: 0x%X\n", patchInfo->origSize, patchInfo->origCRC);
                            printf("     NewSize: %u byte; NewCRC: 0x%X\n\n", patchInfo->newSize, patchInfo->newCRC);
                        }

                        patchReader.ReadNext();
                        patchInfo = patchReader.GetCurInfo();
                        index++;
                    }

                    PrintError(patchReader.GetError());
                }
            }
        }
        else if (command == applyOptions.GetCommand())
        {
            paramsOk = applyOptions.Parse(cmdLine);
            if (paramsOk)
            {
                DAVA::uint32 indexToApply = 0;
                DAVA::FilePath patchPath = applyOptions.GetArgument("PatchFile");
                DAVA::String patchIndex = applyOptions.GetArgument("PatchIndex");
                DAVA::FilePath origPath = applyOptions.GetOption("-i").AsString();
                DAVA::FilePath newPath = applyOptions.GetOption("-o").AsString();
                DAVA::FilePath origBasePath = applyOptions.GetOption("-bo").AsString();
                DAVA::FilePath newBasePath = applyOptions.GetOption("-bn").AsString();
                bool verbose = applyOptions.GetOption("-v").AsBool();

                if (!patchPath.Exists())
                {
                    printf("No such file %s\n", patchPath.GetAbsolutePathname().c_str());
                    ret = 1;
                }

                if (!origBasePath.IsEmpty() && !origBasePath.IsDirectoryPathname())
                {
                    printf("Bad original base dir\n");
                    ret = 1;
                }

                if (!newBasePath.IsEmpty() && !newBasePath.IsDirectoryPathname())
                {
                    printf("Bad new base dir\n");
                    ret = 1;
                }

                if (0 == ret)
                {
                    if (sscanf(patchIndex.c_str(), "%u", &indexToApply) > 0)
                    {
                        DAVA::uint32 index = 0;
                        DAVA::PatchFileReader patchReader(patchPath, verbose);
                        bool indexFound = false;

                        patchReader.ReadFirst();
                        const DAVA::PatchInfo* patchInfo = patchReader.GetCurInfo();
                        while (NULL != patchInfo)
                        {
                            if (index == indexToApply)
                            {
                                if (origPath.IsEmpty())
                                {
                                    origPath = patchInfo->origPath;
                                }

                                if (newPath.IsEmpty())
                                {
                                    newPath = patchInfo->newPath;
                                }
                                else if (newPath.IsDirectoryPathname())
                                {
                                    newPath += patchInfo->newPath;
                                }

                                indexFound = true;
                                ret = DoPatch(&patchReader, origBasePath, origPath, newBasePath, newPath);
                                break;
                            }

                            patchReader.ReadNext();
                            patchInfo = patchReader.GetCurInfo();
                            index++;
                        }

                        if (!indexFound)
                        {
                            printf("No such index - %u\n", indexToApply);
                            ret = 1;
                        }
                    }
                }
            }
        }
        else if (command == applyAllOptions.GetCommand())
        {
            paramsOk = applyAllOptions.Parse(cmdLine);
            if (paramsOk)
            {
                DAVA::FilePath patchPath = applyAllOptions.GetArgument("PatchFile");
                DAVA::FilePath origBasePath = applyAllOptions.GetOption("-bo").AsString();
                DAVA::FilePath newBasePath = applyAllOptions.GetOption("-bn").AsString();
                bool verbose = applyAllOptions.GetOption("-v").AsBool();
                bool truncate = applyAllOptions.GetOption("-t").AsBool();

                if (!patchPath.Exists())
                {
                    printf("No such file %s\n", patchPath.GetAbsolutePathname().c_str());
                    ret = 1;
                }

                if (!origBasePath.IsEmpty() && !origBasePath.IsDirectoryPathname())
                {
                    printf("Bad original base dir\n");
                    ret = 1;
                }

                if (!newBasePath.IsEmpty() && !newBasePath.IsDirectoryPathname())
                {
                    printf("Bad new base dir\n");
                    ret = 1;
                }

                if (0 == ret)
                {
                    DAVA::PatchFileReader patchReader(patchPath, verbose);

                    // go from last patch to the first one and truncate applied patch
                    if (truncate)
                    {
                        patchReader.ReadLast();
                        const DAVA::PatchInfo* patchInfo = patchReader.GetCurInfo();
                        while (NULL != patchInfo && 0 == ret)
                        {
                            ret = DoPatch(&patchReader, origBasePath, DAVA::FilePath(), newBasePath, DAVA::FilePath());

                            patchReader.Truncate();
                            patchReader.ReadPrev();
                            patchInfo = patchReader.GetCurInfo();
                        }
                    }
                    // go from first to the last once and apply patch
                    else
                    {
                        patchReader.ReadFirst();
                        const DAVA::PatchInfo* patchInfo = patchReader.GetCurInfo();
                        while (NULL != patchInfo && 0 == ret)
                        {
                            ret = DoPatch(&patchReader, origBasePath, DAVA::FilePath(), newBasePath, DAVA::FilePath());

                            patchReader.ReadNext();
                            patchInfo = patchReader.GetCurInfo();
                        }
                    }

                    PrintError(patchReader.GetError());
                }
            }
        }
    }

    if (!paramsOk)
    {
        printf("Usage: ResourcePatcher <command>\n");
        printf("\n Commands: write, list, apply, apply-all\n\n");
        printf("%s\n\n", writeOptions.GetUsageString().c_str());
        printf("%s\n\n", listOptions.GetUsageString().c_str());
        printf("%s\n\n", applyOptions.GetUsageString().c_str());
        printf("%s\n\n", applyAllOptions.GetUsageString().c_str());
    }

    return ret;
}

int DAVAMain(Vector<String> cmdLine)
{
    Assert::AddHandler(Assert::DefaultLoggerHandler);
    Assert::AddHandler(Assert::DefaultDebuggerBreakHandler);

    Engine e;
    e.Init(eEngineRunMode::CONSOLE_MODE, {}, nullptr);
    e.update.Connect([&e](float32)
                     {
                         int retCode = Process(e);
                         e.QuitAsync(retCode);
                     });
    return e.Run();
}
