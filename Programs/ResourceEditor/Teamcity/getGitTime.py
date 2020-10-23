import os
import sys

if len(sys.argv) > 1:
	branch = sys.argv[1]
else:
	branch = ' '
	

os.chdir('../')
os.system("git config --global log.date local")

os.chdir('../../')
os.system("git log -1 --format=\"%ci\" > gitTime.txt")
