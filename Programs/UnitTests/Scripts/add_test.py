#!/usr/bin/env python

import os
import sys
import shutil

if len(sys.argv) != 2:
	print """usage:
add_test.py TestClassName"""
	quit()

test_class_name = sys.argv[1]

#source
cpp_read = "Sources/Infrastructure/TestTemplate.cpp.template"
cpp_write = "Sources/Tests/"+test_class_name+".cpp"

with open(cpp_write, 'w') as outfile:
    with open(cpp_read, 'r') as infile:
    	data = infile.read()
    	data = data.replace("$UNITTEST_CLASSNAME$", test_class_name)
    	outfile.write(data)
