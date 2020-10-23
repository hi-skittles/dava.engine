import sys  
import  re
import  os
import glob, os
import subprocess
import argparse
import fnmatch
from contextlib import closing
from zipfile import ZipFile, ZIP_DEFLATED

def zipdir(base_dir, archive_name, contain_directory, ignore_file_masks = []):
    base_dir=os.path.abspath(base_dir) 
    basename=os.path.basename(base_dir)

    dir_archive_name = os.path.dirname(archive_name)
    if not os.path.exists(dir_archive_name):
        os.makedirs(dir_archive_name)

    assert os.path.isdir(base_dir)
    with closing(ZipFile(archive_name, "w", ZIP_DEFLATED)) as z:
        for root, dirs, files in os.walk(base_dir):
            #NOTE: ignore empty directories
            for fn in files:
                absfn = os.path.join(root, fn)
                archived = True
                
                if archive_name in fn:
                   archived = False

                for mask in ignore_file_masks:
                    if fnmatch.fnmatch(fn, mask) or mask in absfn:
                        archived = False
                        break

                if not archived:
                    continue
                
                zfn = absfn[len(base_dir)+len(os.sep):] #XXX: relative path
                if contain_directory :
                    zfn = os.path.join(basename, zfn)
                z.write( absfn, zfn)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument( '--base_dir', action='store' )
    parser.add_argument( '--archive_name', action='store' )
    parser.add_argument( '--contain_directory', action="store_true" )
    args   = parser.parse_args()

    zipdir( args.base_dir, args.archive_name, args.contain_directory )


if __name__ == '__main__':
    main()
