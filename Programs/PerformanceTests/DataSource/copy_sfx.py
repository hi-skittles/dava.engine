#!/usr/bin/env python

import os
import os.path
import sys
import shutil

arguments = sys.argv[1:]

if len(arguments) == 1:
	subDir = arguments[0]

	print "Copy Sfx..."

	currentDir = os.getcwd(); 
	sfxDir =  currentDir + "/../Data/Sfx/"
	sfxSourceDir = currentDir + "/Sfx/" + subDir + "/"

	if not os.path.exists(sfxDir):
		os.makedirs(sfxDir)
		

	removefilelist = [ f for f in os.listdir(sfxDir) if f.endswith(".fev") or f.endswith(".fsb") ]
	for f in removefilelist:
	    os.remove(sfxDir + f)
	
	copyfilelist = [ f for f in os.listdir(sfxSourceDir) if f.endswith(".fev") or f.endswith(".fsb") ]
	for f in copyfilelist:
		shutil.copy(sfxSourceDir + f, sfxDir)

	print "Sfx copied"
else:
    print "error: platform not specified (iOS, Android)"
    exit(1)