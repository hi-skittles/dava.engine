import sys  
import glob, os
import re
import argparse
import subprocess
from contextlib import closing

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
    parser.add_argument( '--dava_path' )
    parser.add_argument( '--build_number' )

    options      = parser.parse_args()

    archiveName  = 'Desc_' + ArchiveName( options.app_name, options.dava_path, options.build_number )

    outPath      = os.path.join( options.out_path, archiveName ) + '.txt'

    if not os.path.exists( options.out_path ):
        os.makedirs( options.out_path )


    open(outPath, "w" ).close()

    print 'Pack options.app_name -> ', outPath 

if __name__ == '__main__':
    main()

