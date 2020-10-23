import argparse
import shutil
import sys  
import glob, os
import re
import argparse
import subprocess

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument( 'folder_path' )
    options      = parser.parse_args()
    if os.path.exists( options.folder_path ):
        print 'delete folder ', options.folder_path
        shutil.rmtree( options.folder_path, ignore_errors=True )

if __name__ == '__main__':
    main()

