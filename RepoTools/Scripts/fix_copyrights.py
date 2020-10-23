#!/usr/bin/env python2.6

import os;
import sys;
import os.path;
import pickle;
import zlib;
import string;
import sys;
import subprocess;
import platform;
import re;
import codecs;
  
excludeDirs = [ "Freetype", "Yaml", "ColladaConverter", "ThirdPartyLibs", "Libs", "yaml-cpp", "PSDTool", "IMagickHelperLib", "bullet", "libuv", "freetype", "ThirdParty"]
excludeFiles = ["Classes/Tests/TextSizeTest.cpp"]
includePaths = {}

replaceString = "\
/*==================================================================================\n\
    Copyright (c) 2008, binaryzebra\n\
    All rights reserved.\n\
\n\
    Redistribution and use in source and binary forms, with or without\n\
    modification, are permitted provided that the following conditions are met:\n\
\n\
    * Redistributions of source code must retain the above copyright\n\
    notice, this list of conditions and the following disclaimer.\n\
    * Redistributions in binary form must reproduce the above copyright\n\
    notice, this list of conditions and the following disclaimer in the\n\
    documentation and/or other materials provided with the distribution.\n\
    * Neither the name of the binaryzebra nor the\n\
    names of its contributors may be used to endorse or promote products\n\
    derived from this software without specific prior written permission.\n\
\n\
    THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS \"AS IS\" AND\n\
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED\n\
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n\
    DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY\n\
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES\n\
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;\n\
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND\n\
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n\
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\n\
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
=====================================================================================*/\n"
	
excludeLogFile = open("excludeLog.log", "w");
includeLogFile = open("includeLog.log", "w");

supported_exts = [".cpp", ".h", ".hpp", ".mm"];

def remove_bom(content):
	if(content.startswith(codecs.BOM_UTF8)):
		return content[len(codecs.BOM_UTF8):]
	return content

def process_contents(content):
	content = remove_bom(content);
	pattern = re.compile("^/[*][=]+[^=]*[=]+[*]/", re.DOTALL);
	if(pattern.search(content) is not None):
		content = pattern.sub('', content);
		content = re.compile('^\s*').sub('', content);
		return replaceString + "\n\n" + content;
	else:
		content = re.compile('^s*').sub('', content);
		if(content.startswith('#')):
			return replaceString + "\n\n" + content;
		else:
			return content;
	
def process_files(arg, dirname, names):
	global excludeDirs, includePaths
	if (string.find(dirname, "$process") != -1):
		return;
	if (string.find(dirname, ".svn") != -1):
		return;
	relPath = os.path.relpath(dirname); 
	for exDir in excludeDirs:	
		if (string.find(relPath, exDir) != -1):
			excludeLogFile.write("exclude: " + relPath + "\n");
			return;

	(dirhead, dirtail) = os.path.split(dirname);
	fullpath = os.path.normpath( dirname + "/");
	for fullname in names:
		pathname = fullpath + "/" + fullname;
		if(
		os.path.isdir(pathname)
		or fullname[0] == '$'
		):
			continue;
		(name, ext) = os.path.splitext(fullname); 
		if ext in supported_exts:
			if any(s.replace("\\", "/") in pathname.replace("\\", "/") for s in excludeFiles):
				print("exclude file detected: " + pathname)
				continue;
			with open(pathname, 'rt') as file:
				content = file.read()
				file.close();
				with open(pathname, 'wt') as file:
					file.write(process_contents(content));
			
	return
pathname = os.path.dirname(__file__);
export_script_dir = os.path.abspath(pathname + "/../../");
os.path.walk(export_script_dir, process_files, None);

excludeLogFile.close();
includeLogFile.close();

#process_file("Animation/AnimatedObject.cpp")