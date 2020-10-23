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
  
excludeDirs = [ "ngt", "cmake", "CMake.app", "Freetype", "Yaml", "ColladaConverter", "ThirdPartyLibs", "Libs", "yaml-cpp", "PSDTool", "IMagickHelperLib", "bullet", "libuv", "freetype", "ThirdParty"]
excludeFiles = ["Classes/Tests/TextSizeTest.cpp"]
includePaths = {}

supported_exts = [".cpp", ".h", ".hpp", ".mm"];

def remove_bom(content):
	if(content.startswith(codecs.BOM_UTF8)):
		return content[len(codecs.BOM_UTF8):]
	return content

def process_contents(content):
	content = remove_bom(content);
	content = str(re.compile('^\s*').sub('', content));
	pattern = re.compile("^/[*][=]+[^=]*[=]+[*]/", re.DOTALL);
	if(pattern.search(content) is not None):
		content = pattern.sub('', content);
		content = re.compile('^\s*').sub('', content);
		return str(content);
	else:
		return str(content);

def process_files(arg, dirname, names):
	global excludeDirs, includePaths
	if (string.find(dirname, "$process") != -1):
		return;
	if (string.find(dirname, ".svn") != -1):
		return;
	relPath = os.path.relpath(dirname); 
	for exDir in excludeDirs:	
		if (string.find(relPath, exDir) != -1):
			print("exclude: ", relPath);
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
