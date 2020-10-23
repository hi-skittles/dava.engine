#!/usr/bin/env python
import sys
import subprocess
import argparse
import texture_dimension_validator as tdv

g_allowed_gpu = ['PowerVR_iOS', 'PowerVR_Android', 'tegra', 'mali', 'adreno', 'dx11', 'origin']
g_default_gpu = 'PowerVR_iOS'

g_allowed_platform = ['iOS', 'Android']

g_default_context = {'validate' : tdv.g_default_mode,
                     'gpu' : g_default_gpu,
                     'HD' : False,
                     'teamcity' : False,
                     'optimized' : False
                     }

def get_input_context():
    global g_default_context
    parser = argparse.ArgumentParser(description='Convert graphics.')
    parser.add_argument('--validate', nargs='?', choices = tdv.g_allowed_mode,
        default = g_default_context['validate'])
    parser.add_argument('--gpu', nargs='?', choices = g_allowed_gpu,
        default = g_default_context['gpu'])
    parser.add_argument('--HD', action='store_true', default=False)
    parser.add_argument('--teamcity', action='store_true', default=False)
    parser.add_argument('--optimized', action='store_true', default=False)
    return vars(parser.parse_args())

def VerboseCall(commands):
    subprocess.call(commands)
    print "subprocess.call " + "[%s]" % ", ".join(map(str, commands))

def do(context=g_default_context):
    if context['gpu'] == 'PowerVR_iOS':
        context['platform'] = 'iOS'
    else:
        context['platform'] = 'Android'

    print context
    cleanUp3d = [sys.executable, 'cleanup_3d.py']
    
    #force param - remove after script refactoring
    if context['teamcity'] and context['validate'] == g_default_context['validate']:
        context['validate'] = tdv.g_mode_strict

    convert3d = [sys.executable, 'convert_3d.py', context['gpu']]
    convert3dTanks = [sys.executable, 'convert_3d_tanks.py', context['gpu']]
    convert3dFX = [sys.executable, 'convert_3d_FX.py', context['gpu']]
    copySfx = [sys.executable, 'copy_sfx.py', context['platform']]

    if context['teamcity']:
        # TODO
        convert3d.append('-teamcity')
        convert3dTanks.append('-teamcity')
        convert3dFX.append('-teamcity')

    VerboseCall(cleanUp3d)
    VerboseCall(convert3d)
    VerboseCall(convert3dTanks)
    VerboseCall(convert3dFX)
    VerboseCall(copySfx)

    if context['platform'] == 'Android':
        print "create apk file system"
        createFileStruct = ["python", "create_file_structure.py", "../Data", "../Data/fileSystem.yaml"]
        VerboseCall(createFileStruct)

if __name__ == '__main__':
    context = get_input_context()
    do(context)
