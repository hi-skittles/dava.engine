import os
import zipfile
import contextlib
import sys
import argparse
import glob, os

def zip_to_dir( archive_name, out_dir ):

    for file in glob.glob( archive_name ):
        with zipfile.ZipFile(file, "r") as z:
            z.extractall( out_dir )
            if sys.platform == "darwin":
                for member in z.infolist():
                    os.chmod( os.path.join( out_dir, member.filename) , 0755 )
        print 'zip_to_dir >>',  file , ' --> ', out_dir

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument( '--archive_name', action='store' )
    parser.add_argument( '--out_dir', action='store' )
    args   = parser.parse_args()

    zip_to_dir( args.archive_name, args.out_dir )


if __name__ == '__main__':
    main()
