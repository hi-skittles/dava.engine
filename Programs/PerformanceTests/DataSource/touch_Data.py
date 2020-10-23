#!/usr/bin/env python

import os
import os.path
import platform
import subprocess

if platform.system() == "Darwin":
	print "touch_Data.py"
	subprocess.call(['touch', '-cm', '../Data/'])
	subprocess.call(['touch', '-cm', '../Data/Configs/'])
else:
	print "skipped touch"