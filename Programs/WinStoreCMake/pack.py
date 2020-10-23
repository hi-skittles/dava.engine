import zipfile
import sys
import os

if len(sys.argv) < 2:
    print 'Usage: Please enter path to zip-file and output folder'
    exit(1)

zipName = sys.argv[2]
folderName = sys.argv[1]

parentFolder = os.path.dirname(folderName)
contents = os.walk(folderName)

zipFile = zipfile.ZipFile(zipName, 'w', zipfile.ZIP_DEFLATED)
for root, folders, files in contents:
    for folderName in folders:
        absolutePath = os.path.join(root, folderName)
        relativePath = absolutePath.replace(parentFolder + '\\', '')
        zipFile.write(absolutePath, relativePath)
    for fileName in files:
        absolutePath = os.path.join(root, fileName)
        relativePath = absolutePath.replace(parentFolder + '\\', '')
        zipFile.write(absolutePath, relativePath)