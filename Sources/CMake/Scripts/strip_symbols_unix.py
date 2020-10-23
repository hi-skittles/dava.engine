#!/usr/bin/env python
import os
import subprocess
import argparse

parser = argparse.ArgumentParser()
parser.add_argument( 'strip_program' )
parser.add_argument( 'path_to_strip' )
args = parser.parse_args()

for rootdir, dirs, files in os.walk( args.path_to_strip ):
    for file in files:   
        if file.endswith( '.so' ):
            full_file = rootdir + '/' + file
            print 'Strip ' + full_file
            call_string = [args.strip_program, '--strip-unneeded', full_file]
            subprocess.check_output(call_string)