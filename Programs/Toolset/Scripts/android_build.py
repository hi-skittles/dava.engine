import argparse
import os
import subprocess
import sys


DavaRootDir = os.path.realpath(os.path.join( os.path.dirname (__file__), '../../../'))
DavaProgramsDir = os.path.join( DavaRootDir, 'Programs' )

ProgramsList = [ 'UnitTests', 
                 'TestBed',
                 'SceneViewer',
                 'UIViewer',
                 'PerformanceTests'
               ]


def build( program ):
    program_dir = os.path.join( DavaProgramsDir, program, 'Platforms', 'Android') 

    os.chdir(program_dir)

    print "===== Building % s =====" % (program)
    
    command = '{}:assembleFatRelease'.format(program);
    bat_file = ''  # empty on UNIX platforms
    if os.name == 'nt':  # windows
        bat_file = '.bat'
    subprocess.check_call([program_dir+'/gradlew'+bat_file, command])

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument( '--sdk_dir' )
    parser.add_argument( '--ndk_dir' )

    options = parser.parse_args()

    exists_sdk_dir = os.path.exists( options.sdk_dir ) 
    exists_ndk_dir = os.path.exists( options.ndk_dir ) 

    assert exists_sdk_dir == True
    assert exists_ndk_dir == True

    sdk_dir = options.sdk_dir
    ndk_dir = options.ndk_dir

    if os.name == 'nt':  # windows
        sdk_dir = sdk_dir.replace('\\', '/')
        ndk_dir = ndk_dir.replace('\\', '/')

    local_properties_str = 'sdk.dir={}\nndk.dir={}\n'.format(sdk_dir, ndk_dir)

    for program in ProgramsList:
        program_dir = os.path.join( DavaProgramsDir, program, 'Platforms', 'Android') 

        os.chdir(program_dir)


        with open( 'local.properties', 'w' ) as local_properties_file:
            local_properties_file.write(local_properties_str)
        
        build(program)

if __name__ == '__main__':
    main()

