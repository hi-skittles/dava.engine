#!/usr/bin/env python
#
#  DAVA SDK
#
#  Copyright (c) 2013 DAVA Consulting, LLC. All rights reserved.
#

import sys
import os

path = ""
outFile = ""

if len(sys.argv) < 3:
  path = "../Data"
  outFile = "../Data/fileSystem.yaml"
  print "No input path, using default settings: " + path + " " + outFile
else:
  path = sys.argv[1]  
  outFile = sys.argv[2]
  
path = os.path.abspath(path)
dirPath = os.path.basename(path)

dirList = [dirPath]
fileList = []

def getFiles(path, dirPath):
  global dirList
  global fileList

  files = os.listdir(path)
  for file in files:
    if (os.path.isdir(path + "/" + file)):
      dirList += [dirPath + "/" + file]
      getFiles(path + "/" + file, dirPath + "/" + file)
    else:
      fileList += [dirPath + "/" + file]

def writeFile(f, tag, list):
  f.write(tag + ": [")
  itemLen = len(list)
  i = 0
  for item in list:
    f.write("\"" + item + "\"")
    i += 1
    if i != itemLen:
      f.write(", ")
  f.write("]\n")

getFiles(path, dirPath)
f = open(outFile, "w")
writeFile(f, "dirList", dirList)
writeFile(f, "fileList", fileList)
f.close()