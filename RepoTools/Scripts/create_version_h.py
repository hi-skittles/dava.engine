import os
import sys
import string
import argparse
import pack_app

def main():
    parser  = argparse.ArgumentParser()
    parser.add_argument( '--dava_path',    required = True )
    parser.add_argument( '--build_number', required = True )
    parser.add_argument( '--branch_info' )

    options = parser.parse_args()

##    
    pathFolderVersion = os.path.join(options.dava_path, 'Modules/Version/Sources/Version/Private')
    pathFileVersion   = os.path.join(pathFolderVersion, 'VersionDefine.h')

    titleName         = pack_app.ArchiveName( None, options.dava_path, options.build_number ) 

    os.chdir( pathFolderVersion )

    if options.branch_info :
        branch_info = options.branch_info
        titleName += '_[ {0} ]'.format( branch_info )

    print 'Title name - ', titleName
    defineString = '#define APPLICATION_BUILD_VERSION "{0}"'.format( titleName )

##
    myFile  = open( pathFileVersion, 'w')
    myFile.write( defineString )
    myFile.close() 


if __name__ == '__main__':
    main()








