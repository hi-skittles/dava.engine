import argparse
import shutil
import sys  
import glob, os
import re
import argparse
import subprocess

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument( 'in_path' )
    parser.add_argument( 'out_path' )

    options      = parser.parse_args()
    if os.path.exists( options.in_path ):
        print 'rename ', options.in_path, ' to ', options.out_path
        os.rename( options.in_path, options.out_path )

if __name__ == '__main__':
    main()

