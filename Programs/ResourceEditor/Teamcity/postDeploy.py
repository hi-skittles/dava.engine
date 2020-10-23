import os
import sys
from subprocess import Popen, PIPE
from shutil import copytree, rmtree, ignore_patterns, copyfile


def CopyFolder(srcFolder, dstFolder):
    print "Copy folder " + srcFolder + " to " + dstFolder

    IGNORE_PATTERNS = ('.svn')
    copytree(srcFolder, dstFolder, symlinks=False, ignore=ignore_patterns(IGNORE_PATTERNS))

def main():
    if len(sys.argv) < 2:
        print "Usage : postDeploy.py <deploy_root_folder> [beast_lib_folder]"
        exit()

    deployRoot = sys.argv[1]

    proc = Popen("git log --since=3.days --branches=\"development\" --pretty=format:\"%s (%an, %ar)\"", shell=True, stdout=PIPE)
    changesFile = open(os.path.join(deployRoot, "changes.txt"), 'w+')
    for line in proc.stdout:
        changesFile.write(line)

    if len(sys.argv) > 2:
        beastFolderPath = sys.argv[2].rstrip("\\/")
        beastDstPath = os.path.join(deployRoot, 'Data.beast')

        print "Beast Path: " + beastFolderPath
        print "DST Path: " + beastDstPath

        if os.path.exists(beastDstPath):
            rmtree(beastDstPath)

        folders = ["/wrapper/config", "/beast/shaders"]
        for folder in folders:
            CopyFolder(beastFolderPath + folder, beastDstPath + folder)

        srcBinFolder = beastFolderPath + "/beast/bin/"
        dstBinFolder = beastDstPath + "/beast/bin/"
        os.mkdir(dstBinFolder)
        filesInBin = ["ernsttool.xml", "ernst-64.exe", "ernsttool-64.exe", "filtertool-64.exe", "oslc-64.exe", "rendertool.xml", "rendertool-32.exe", "rendertool-64.exe", "rendertoolcontroller-64.exe", "thebeast-64.exe", "uvtool-64.exe"]
        for binaryFile in filesInBin:
            srcFile = srcBinFolder + binaryFile
            dstFile = dstBinFolder + binaryFile

            print "Copy file " + srcFile + " to " + dstFile

            copyfile(srcFile, dstFile)

if __name__ == '__main__':
    main()