#!/usr/bin/env python

import fnmatch
import os
import sys
import codecs

def process_file(path):
    with open(path, 'rt') as file:
        content = file.read()
        file.close();
        if(content.startswith(codecs.BOM_UTF8)):
            with open(path, 'wt') as file:
                file.write(content[len(codecs.BOM_UTF8):]);
                file.close();

sources = ['Sources/Internal', 'Modules', 'Programs']
for source in sources:
	for root, dirnames, filenames in os.walk("../../../" + source):
		for ext in ['cpp', 'h', 'mm']:
			for filename in fnmatch.filter(filenames, '*.'+ext):
				file = os.path.join(root, filename)
				process_file(file)
