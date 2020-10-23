#!/usr/bin/env python

import os
import os.path
import shutil

currentDir = os.getcwd(); 

def delete_dir(relativePath):
    deleteDir =  os.path.realpath(currentDir + relativePath)
    if os.path.exists(deleteDir):    
        print "delete " + deleteDir
        shutil.rmtree(deleteDir)

print "delete all $process from DataSource and converted graphics from Data"

delete_dir("/$process")
delete_dir("/Gfx/$process")
delete_dir("/Gfx2/$process")

delete_dir("/../Data/Gfx")
delete_dir("/../Data/Gfx2")

fileSystemYaml =  os.path.realpath(currentDir + "/../Data/fileSystem.yaml")
if os.path.exists(fileSystemYaml):
	print "delete " + fileSystemYaml
	os.remove(fileSystemYaml)