import sys
import zipfile

if len(sys.argv) < 2:
    print 'Usage: Please enter path to zip-file and output folder'
    exit(1)

zipName = sys.argv[1]
folderName = sys.argv[2]

zf = zipfile.ZipFile(zipName)
zf.extractall(folderName)