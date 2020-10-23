#!/usr/bin/env python2.7
import string
import re
import sys

arguments = sys.argv[1:]

path_DAVAVersion  = arguments[0] + '/Sources/Internal/DAVAVersion.h'
version_framework = arguments[1]

pattern = re.compile("DAVAENGINE_VERSION")

f_version     = open( path_DAVAVersion, 'r+' )
lines         = f_version.readlines()
define_engine = '#define DAVAENGINE_VERSION '

for i, line in enumerate(lines):
    for match in re.finditer(pattern, line):
        lines[i] = define_engine + '"{0}"\n'.format( version_framework )

print 'changed to version {0}'.format( version_framework )

f_version.seek(0)
f_version.truncate()
f_version.writelines(lines)
f_version.close()

