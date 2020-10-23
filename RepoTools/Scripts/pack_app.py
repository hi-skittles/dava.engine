import sys  
import glob, os
import re
import argparse
import subprocess
from zip import zipdir
from contextlib import closing
from zipfile import ZipFile, ZIP_DEFLATED
 

def GetGitVersion( pathToFramework ):
    os.chdir(pathToFramework)
    gitVersion = str(os.popen( 'git log -1 --format=\"%ci\"' ).read())
    gitVersion = gitVersion.rstrip(os.linesep)
    gitVersionArray = gitVersion.split(' ')
    gitVersion = '_'.join( [ gitVersionArray[0], gitVersionArray[1] ]  )      
    gitVersion = re.sub('[:]','-', gitVersion ).rstrip()
    return gitVersion

def ArchiveName( app_name, dava_path, build_number ):

    archiveName = []

    if app_name :
        archiveName  = [ app_name ]

    if dava_path :
        versionGit   = GetGitVersion( dava_path )
        archiveName += [ versionGit ]

    if build_number :
        archiveName  += [ build_number ]

    return '_'.join( archiveName )
    
def main():
    parser = argparse.ArgumentParser()

    parser.add_argument( '--app_name',  required = True )
    parser.add_argument( '--app_path',  required = True )
    parser.add_argument( '--out_path',  required = True )
    parser.add_argument( '--ignore_file_masks' )

    parser.add_argument( '--dava_path' )
    parser.add_argument( '--build_number' )

    options      = parser.parse_args()

    archiveName  = [ options.app_name ]

    ignore_file_masks = []
    if options.ignore_file_masks:
        ignore_file_masks = options.ignore_file_masks.split(' ')

    if options.dava_path :
        versionGit   = GetGitVersion ( options.dava_path ) 
        archiveName += [ versionGit ]

    if options.build_number :
        archiveName  += [ options.build_number ]

    outPath      = os.path.join( options.out_path, '_'.join(archiveName) ) + '.zip'

    if not os.path.exists( options.out_path ):
        os.makedirs( options.out_path )

    if os.path.exists( options.app_path ):
        print 'Pack options.app_name -> ', outPath
        zipdir( options.app_path, outPath, False, ignore_file_masks )
    else:
       print 'No packing folder -> ', options.app_path


if __name__ == '__main__':
    main()

