import re
import os
import sys

os.system("del /q ..\\_ReleaseQt\\*.zip")

file_git_time="gitTime.txt"
os.chdir('../../../')
data = open(file_git_time).read()
commitNumber = re.sub('[: +]','_', data).rstrip()

os.chdir('Tools/ResourceEditor/Teamcity')
os.system("packAppQt.bat " + commitNumber)
os.system("xcopy /y ..\\_ReleaseQt\\ResourceEditor_win_" + commitNumber + ".zip \\\\winserver\win\development")

file_in = '\\\\winserver\\win\\launcher\\configs\\development.yaml'
file_out = 'development_temp.yaml'

output_file = open(file_out,"w")
data = open(file_in).read()
output_file.write( re.sub('ResourceEditor_win_.*\.zip','ResourceEditor_win_' + commitNumber + '.zip', data)  )
output_file.close()
os.remove(file_in)

output_file = open(file_in,"w")
data = open(file_out).read()
output_file.write( re.sub('ver: .*\n','ver: ' + commitNumber + '\n', data)  )
output_file.close()
os.remove(file_out)
