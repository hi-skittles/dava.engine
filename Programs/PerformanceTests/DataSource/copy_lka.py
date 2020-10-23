#!/usr/bin/env python
#
#  copy_lka.py
#  DAVA SDK
#
#  Created by Yury Danilov on 6/14/12.
#  Copyright (c) 2012 DAVA Consulting, LLC. All rights reserved.
#



import os;
import sys;
import os.path;
import string;
import sys;
import shutil

def loadSimpleYamlAsMap(fileName):
    f = open(fileName)
    fileLines = f.readlines()
    f.close()
    data = {}
    for fileLine in fileLines:
        strippedLine = string.strip(fileLine)
        if 0 != string.find(strippedLine, "#"):
            lineWords = string.split(strippedLine, ": ")
            if len(lineWords)==2:
                data[lineWords[0]] = string.strip(lineWords[1], '"')
    return data

arguments = sys.argv[1:]

if 0 == len(arguments) or (len(arguments) < 2 or 3 < len(arguments)):
    print "usage: ./copy_lka.py RelativePathToMapsSrc RelativePathToMapsDest [RelativePathToMapsList]"
    exit(1)
    
currentDir = os.getcwd()

relativePathToMapsSrc = arguments[0]
relativePathToMapsDest = arguments[1]

relativePathToMapsList = '../Data/maps.yaml'

if 3 == len(arguments):
    relativePathToMapsList = arguments[2]
    print "relativePathToMapsList=" + relativePathToMapsList

# read and process maps from maps.yaml
dataMap = loadSimpleYamlAsMap(relativePathToMapsList)
    
for (serverMap, localMap) in dataMap.iteritems():
    if serverMap != "default":
        lkaSrcPath = os.path.realpath(relativePathToMapsSrc + "/" + str(localMap) + '.lka')
        lkaDestPath = os.path.realpath(relativePathToMapsDest + "/"  + str(localMap) + '.lka')
        
        (lkaDestFolder, lkaDestFileName) = os.path.split(lkaDestPath)
        # copy mapPath.lka
        if os.path.exists(lkaSrcPath):
            if not os.path.exists(lkaDestFolder):
                os.makedirs(lkaDestFolder)
            if os.path.exists(lkaDestPath):
                print "delete " + lkaDestPath
                os.remove(lkaDestPath)
            print "copy " + lkaSrcPath + " to " + lkaDestPath
            shutil.copy(lkaSrcPath, lkaDestPath)
# come back
os.chdir(currentDir)
