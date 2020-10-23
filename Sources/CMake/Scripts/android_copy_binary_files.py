#!/usr/bin/env python
import os
import argparse
import shutil

parser = argparse.ArgumentParser()
parser.add_argument( 'input_path' )
parser.add_argument( 'output_path' )
args = parser.parse_args()

input_path = os.path.abspath(args.input_path)
output_path = os.path.abspath(args.output_path)

for rootdir, dirs, files in os.walk( args.input_path ):
    for file in files:
        input_file_name = os.path.abspath(rootdir + '/' + file)
        output_file_name = ''
        
        if file.endswith( 'CMakeCCompilerId.o' ) or file.endswith( 'CMakeCXXCompilerId.o' ):
            continue
            
        elif file.endswith( '.a' ) or file.endswith( '.so' ):
            output_file_name = output_path + '/' + file
        
        elif file.endswith( '.o' ):
            output_file_name = output_path + '/objs/' + os.path.relpath(input_file_name, input_path)
            
        if output_file_name:
            output_file_dir_name = os.path.dirname(output_file_name)
            if not os.path.exists(output_file_dir_name):
                os.makedirs(output_file_dir_name)
                
            print 'Copy object file: ' + os.path.basename(output_file_name) + ' to ' + output_file_dir_name
            shutil.copyfile(input_file_name, output_file_name)