#!/usr/bin/env python
import sys
import os
import subprocess
import argparse
import platform

parser = argparse.ArgumentParser()

parser.add_argument( 'cmake_command' )
parser.add_argument( 'path_prj' )
parser.add_argument( 'cur_vers' )
parser.add_argument( 'dir_list', nargs='+' )

args     = parser.parse_args()
dir_list = ' '.join(args.dir_list)

current_dir       = os.path.dirname(os.path.realpath(__file__)) + '/'
CURRENT_VERSIONS  = str(os.popen( 'python ' + current_dir + 'file_tree_hash.py ' + dir_list ).read())  
LAST_VERSIONS     = str(args.cur_vers)    
CURRENT_VERSIONS  = CURRENT_VERSIONS.rstrip(os.linesep)

print 'VersionsCheck ------ ', args.dir_list
print 'CURRENT_VERSIONS ', CURRENT_VERSIONS
print 'LAST_VERSIONS    ', LAST_VERSIONS

if ( CURRENT_VERSIONS != LAST_VERSIONS ) :
    cmake_program = args.cmake_command
    sys.stdout.write( 'Update cmake project\n' )

    call_string = [cmake_program, args.path_prj]
    subprocess.check_output(call_string)

    sys.stdout.write( 'error: Automatic update cmake project. Please rebuild\n' )
    sys.stdout.flush()

sys.exit(CURRENT_VERSIONS != LAST_VERSIONS)