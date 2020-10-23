#!/usr/bin/python
import shutil
import os
from os.path import expanduser

app = "QuickEd"
appPath = app + ".app"
frameworkPath = appPath + "/Contents/Frameworks/"
qtPath = expanduser("~") + "/QtSDK/Desktop/Qt/4.8.1/gcc/lib/"

if os.path.isdir(frameworkPath):
	shutil.rmtree(frameworkPath)

os.makedirs(frameworkPath)

#copy framework
def CopyFramework(lib):
	os.makedirs(frameworkPath + lib + ".framework")
	shutil.copyfile(qtPath + lib + ".framework/" + lib, frameworkPath + lib + ".framework/" + lib)
	if os.path.isdir(qtPath + lib + ".framework/" + "Resources"):
		shutil.copytree(qtPath + lib + ".framework/" + "Resources", frameworkPath + lib + ".framework/" + "Resources")
	
CopyFramework("QtCore")
CopyFramework("QtGui")
CopyFramework("QtOpenGL")
CopyFramework("QtNetwork")

def ChangeAlias(exe, lib):
	cmd = qtPath + lib + ".framework/Versions/4/" + lib + " " + \
	 	"@executable_path/../Frameworks/" + lib + ".framework/" + lib + " " + exe
	os.system("install_name_tool -change " + cmd)

ChangeAlias(appPath + "/Contents/MacOS/" + app, "QtCore")
ChangeAlias(appPath + "/Contents/MacOS/" + app, "QtGui")
ChangeAlias(appPath + "/Contents/MacOS/" + app, "QtOpenGL")
ChangeAlias(appPath + "/Contents/MacOS/" + app, "QtNetwork")

ChangeAlias(frameworkPath + "QtGui.framework/QtGui", "QtCore")

ChangeAlias(frameworkPath + "QtOpenGL.framework/QtOpenGL", "QtCore")
ChangeAlias(frameworkPath + "QtOpenGL.framework/QtOpenGL", "QtGui")

ChangeAlias(frameworkPath + "QtNetwork.framework/QtNetwork", "QtCore")

os.system("otool -L " + frameworkPath + "QtCore.framework/QtCore")
os.system("otool -L " + frameworkPath + "QtGui.framework/QtGui")
os.system("otool -L " + frameworkPath + "QtOpenGL.framework/QtOpenGL")
os.system("otool -L " + frameworkPath + "QtNetwork.framework/QtNetwork")
os.system("otool -L " + appPath + "/Contents/MacOS/" + app)