import os
import sys
from subprocess import Popen, PIPE
from optparse import OptionParser

parser = OptionParser()
parser.add_option("-p", action="store", dest="platform", help="Platform where deploy processing : WIN, MAC")
parser.add_option("-q", action="store", dest="qtPath", help="Path to Qt root folder")
parser.add_option("-d", action="store", dest="deployRoot", help="Root folder of build. {_build/app} for example")
parser.add_option("-a", action="store", dest="deployArgs", help="Platform specific arguments that put into deplot qt util")
parser.add_option("-n", action="store", dest="toolName", help="Tool name")
parser.add_option("-t", action="store", dest="targetsList", help="Target lists")


(options, args) = parser.parse_args()

if not options.qtPath or not options.deployRoot or not options.deployArgs or not options.platform:
    parser.print_help()
    exit()

deployUtilName = ""
if options.platform == "WIN":
    os.environ['PATH'] = os.path.join(options.qtPath, "bin")
    deployUtilName = "windeployqt"
elif options.platform == "MAC":
    deployUtilName = options.qtPath.rstrip("\\/") + "/bin/macdeployqt"
else:
    parser.print_help()
    exit()
prevCurrentDir = os.getcwd()
os.chdir(options.deployRoot)

if options.platform == "MAC":
    execDir            = os.path.join( options.deployRoot, '{0}.app/Contents/MacOS'.format( options.toolName ) )
    pathFileExecute    = os.path.join( execDir, options.toolName  )
    pathFileExecuteTmp = os.path.join(execDir, 'Root_{0}'.format( options.toolName ) )
    os.rename(pathFileExecute, pathFileExecuteTmp )
    for rootdir, dirs, files in os.walk( execDir ):
        for file in files:
            pathFileProcessed =  os.path.join(rootdir,  file  )
            os.rename( pathFileProcessed, pathFileExecute )
            print 'Qt deploy - ', file
            sys.stdout.flush()
            process = Popen(deployUtilName + " " + options.deployArgs, shell=True, stdout=PIPE)
            for line in process.stdout:
                print line
            os.rename( pathFileExecute, pathFileProcessed )
    os.rename( pathFileExecuteTmp, pathFileExecute )
else:
    if options.targetsList is None:
        targetsList = [ options.toolName ]
    else:
        targetsList = options.targetsList.split(';')

    for target in targetsList:
        print 'Qt deploy - ', target
        sys.stdout.flush()
        pathFileExecute = os.path.join( options.deployRoot, '{0}.exe'.format( target ) )
        pathFileExecute = os.path.realpath( pathFileExecute )
        deployArgs = options.deployArgs + ' {0}'.format( pathFileExecute )
        process = Popen(deployUtilName + " " + deployArgs, shell=True, stdout=PIPE)
        print deployArgs

os.chdir(prevCurrentDir)
