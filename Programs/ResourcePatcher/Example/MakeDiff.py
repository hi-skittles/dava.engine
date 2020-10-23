import zipfile
import sys
import tempfile
import subprocess
import os
import shutil
from optparse import OptionParser


#Global variables ------------------------------------------

#it will be ideal to define that variables from outer context

resourcePatcherPath = ""
if "win32" == sys.platform:
    resourcePatcherPath = "../Debug/ResourcePatcher.exe"
else:
    if "darwin" == sys.platform:
        resourcePatcherPath = "./../Build/Products/Debug/ResourcePatcher"
        
resourcesVersionFileName = "Data1.txt"
gameVersionFileName = "Root1.txt"
relativeResourcesInfoFilePath = "Root/Data/"
relativeGameInfoFilePath = "Root/"
#end of Global Variables --------------------------------------


def ExtractArchive(filePath):
    tmpDir = tempfile.mkdtemp() + "/"
    try: #extract file
        with zipfile.ZipFile(filePath) as zf:
            zf.extractall(tmpDir)
    except zipfile.ZipFile:
        print "bad ZIP archive as base file. Location:", filePath
    return tmpDir

def GetResourceVersion(resourceFolderPath):
    try:
        with open(resourceFolderPath, 'r') as resourceVersionFile:
            return resourceVersionFile.read()
    except (OSError, IOError) as e:
        print "Resources Version File is not opened"
        return ""

def GetGameVersion(gameVersionInfoFilePath):
    try:
        with open(gameVersionInfoFilePath, 'r') as gameVersionFile:
            return gameVersionFile.read()
    except (OSError, IOError) as e:
        print "Game Version File is not opened"
        return ""

def MakeVersionString(gameVerStr, resVerStr):
    return gameVerStr + "_" + resVerStr

def GetFullVersion(gameRootFolder):
    resourcesVersion = ""
    gameVersion = ""

    resourcesVersion = GetResourceVersion(gameRootFolder + relativeResourcesInfoFilePath + resourcesVersionFileName)
    gameVersion = GetGameVersion(gameRootFolder + relativeGameInfoFilePath + gameVersionFileName)

    return MakeVersionString(gameVersion, resourcesVersion)

def GetPatchNameFromBuildVersions(baseVer, newVer):
    return "patch_from_" + baseVer + "_to_" + newVer + ".patch"
    

# Main script

parser = OptionParser()
parser.add_option("-b", dest="base_file", metavar="baseFilename")
parser.add_option("-n", dest="new_file", metavar="newFilename")
parser.add_option("-p", dest="patch_file", metavar="patchFilename")

(options, args) = parser.parse_args(sys.argv)

baseFilename = options.base_file
newFilename = options.new_file
patchFilename = options.patch_file

#temporary directories for extracted files
tmpDirBase = ExtractArchive(baseFilename)
tmpDirNew = ExtractArchive(newFilename)

#we could to override patchFilename
if (None == patchFilename):
    baseBuildVersion = GetFullVersion(tmpDirBase)
    newBuildVersion = GetFullVersion(tmpDirNew)
    patchFilename = GetPatchNameFromBuildVersions(baseBuildVersion, newBuildVersion)    

try: #call resource patcher
    subprocess.check_call([resourcePatcherPath, "write", tmpDirBase, tmpDirNew, patchFilename])
except subprocess.CalledProcessError as e:
    print "patcher returns", e.returncode

#remove temporary directories
shutil.rmtree(tmpDirBase)
shutil.rmtree(tmpDirNew)
