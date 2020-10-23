
import re
import os
import sys

file_in = '\\\\winserver\\win\\launcher\\configs\\development.yaml'
file_out = 'development_temp.yaml'

output_file = open(file_out,"w")
data = open(file_in).read()
output_file.write( re.sub('ResourceEditor_win_.*\.zip','ResourceEditor_win_' + sys.argv[1] + '.zip', data)  )
output_file.close()
os.remove(file_in)

output_file = open(file_in,"w")
data = open(file_out).read()
output_file.write( re.sub('ver: .*\n','ver: ' + sys.argv[1] + '\n', data)  )
output_file.close()
os.remove(file_out)

#os.remove(file_in)
#os.rename(file_out,file_in)
