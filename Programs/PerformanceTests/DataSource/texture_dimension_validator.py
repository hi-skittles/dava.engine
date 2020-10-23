#!/usr/bin/env python

import os
import os.path
import argparse
import sys # exit
import logging
import struct
import pickle

g_logger = None

g_mode_force_update = 'force_update'
g_mode_interactive = 'interactive'
g_mode_strict = 'strict'
g_mode_skip = 'skip'
g_allowed_mode = [g_mode_force_update, g_mode_interactive, g_mode_strict, g_mode_skip]

g_default_mode = g_mode_interactive
g_default_DataSource = "./"
g_default_Data = g_default_DataSource + "../Data/"
g_default_context = {'validate' : g_default_mode, 
                     'data_source' : g_default_DataSource,
                     'data' : g_default_Data,
                     'dirs' : ['Gfx', 'Gfx2']
                     }

g_allowed_texture_format = ['.png', '.dds', '.pvr']
g_GPU_Suffix = ['.PowerVR_iOS', '.PowerVR_Android', '.tegra', '.mali', '.adreno']
g_texture_info_ext = '_size.txt'

def show_context(context):
    g_logger.info("Verification mode:\t" + context['validate'])
    g_logger.info("DataSource path:\t"   + context['data_source'])
    g_logger.info("Data path:\t\t"       + context['data'])
    g_logger.info("path:\t\t"           + str(context['dirs']))

def get_input_context():
    allowed_mode_help = "Allowed verificator mode: "
    allowed_mode_help += "force_update - generate description files; "
    allowed_mode_help += "interactive - if a change is detected ask user what should do; "
    allowed_mode_help += "strict - verification failed if a change is detected"
    allowed_mode_help += "skip - do nothing"

    parser = argparse.ArgumentParser(description='verify texture description files.')
    parser.add_argument('-validate', nargs='?', choices=g_allowed_mode, 
                    default = g_default_context['validate'], help=allowed_mode_help)
    parser.add_argument('-data_source', required=False, default=g_default_context['data_source'],
                        help='Path to DataSource folder. Default: ' + g_default_DataSource)
    parser.add_argument('-data', required=False, default=g_default_context['data'], 
                        help='Path to Data folder. Default: ' + g_default_Data)

    parser.add_argument('-dirs', nargs='?', default = g_default_context['dirs'])

    try:
        return vars(parser.parse_args())
    except Exception, e:
        sys.exit("Parsing arguments is failed: " + e.message)

def png_get_size(path):
    file = open(path, 'rb')
    header_raw = file.read(8)
    if header_raw[1:4] != 'PNG':
        raise ValueError("Is not a PNG file")
    header_raw = file.read(18)
    if header_raw[4:8] != 'IHDR':
        raise ValueError("Wrong chunk.")
    header = struct.unpack('!IbbbbllBB', header_raw)
    file.close()
    return header[5]*header[6], header[5], header[6]

def dds_get_size(path):
    file = open(path, 'rb')
    header_raw = file.read(128)
    file.close()
    if header_raw[:4] != 'DDS ':
        raise ValueError("Is not a DDS file")
    header = struct.unpack('<IIIIIII 44x 32s 16s 4x', header_raw[4:])
    return header[3]*header[2], header[3], header[2]

def PVR_GetSize(path):
    file = open(path, 'rb')
    header_raw = file.read(52)
    file.close()
    hdr = header_raw[:3]
    if header_raw[:3] != 'PVR':
        raise ValueError("Is not a PVR file")
    header = struct.unpack_from('I I Q I I I I I I I I', header_raw)
    #pvr3: height*width
    return header[6]*header[5], header[6], header[5]

def get_info_for_texture(path):
    info = None
    try:
        filename, ext = os.path.splitext(path)
        if ext == ".pvr":
            info = PVR_GetSize(path)
        elif ext == ".dds":
            info = dds_get_size(path)
        else:
            info = png_get_size(path)
    except Exception, e:
        g_logger.debug("Cannot get texture info for: " + path + " Reason: " + e.message)

    return info

def write_info(info, path):
    dir = os.path.dirname(path)
    if not os.path.exists(dir):
        raise IOError("Directory does not exist: " + dir)
    with open(path, 'w') as handle:
        pickle.dump(info, handle)

def read_info(path):
    data = None
    try:
        file = open(path, 'r')
        data = pickle.load(file)
        file.close()
    except Exception, e:
        g_logger.error("Cannot read texture info: " + path)
        g_logger.error(e)
    return data

def equal_info(i1, i2):
    isNone = (i1 == None or i2 == None)
    return not isNone and (i1[0] == i2[0])

def strict_equal_info(i1, i2):
    isNone = (i1 == None or i2 == None)
    return not isNone and (i1[0] == i2[0]) and (i1[1] == i2[1]) and (i1[2] == i2[2])

def tostr(i):
    if i == None:
        return "Empty"
    return str(i[0]) + "=[" + str(i[1]) + "x" + str(i[2]) + "]"

def verify(mode, dRootPath, dsRootPath):
    errNumber = 0
    for root, dir, files in os.walk(dRootPath):
        for file in files:
            filename, ext = os.path.splitext(file)
            if not ext in g_allowed_texture_format:
                continue

            dPathTexture = os.path.join(root, file)
            # info = [width, heigh]
            info = get_info_for_texture(dPathTexture)
            # dPathTexture - Data/Example/Texture.PowerVR_iOS.pvr
            relFilePath = os.path.splitext(dPathTexture.strip(dRootPath + os.sep))[0]
            for suffix in g_GPU_Suffix:
                if relFilePath.find(suffix) != -1:
                    relFilePath = relFilePath.replace(suffix, '')
                    break
            # dsPathTxt - DataSource/Example/Texture_size.txt
            dsPathTxt = os.path.join(dsRootPath, relFilePath) + g_texture_info_ext

            # rewrite all info
            if mode == g_mode_force_update:
                g_logger.debug("Force write info: " + dsPathTxt)
                write_info(info, dsPathTxt)
                continue

            oldInfo = read_info(dsPathTxt)
            if equal_info(info, oldInfo):
                g_logger.debug("Info equals: " + dPathTexture + ", Txt: " + dsPathTxt)
                if not strict_equal_info(info, oldInfo):
                    g_logger.warning("Info equals, but dimension are different: ")
                    g_logger.warning("Texture file: " + dPathTexture)
                    g_logger.warning("Old info: " + tostr(oldInfo))
                    g_logger.warning("New info: " + tostr(info))
                continue

            g_logger.warning("Texture file: " + dPathTexture)
            g_logger.warning("Old info: " + tostr(oldInfo))
            g_logger.warning("New info: " + tostr(info))

            if mode == g_mode_strict:
                g_logger.error("Failed.")
                errNumber = errNumber + 1
            else:
                g_logger.warning("Rewrite texture file info: " + dsPathTxt)
                write_info(info, dsPathTxt)

    return errNumber

# {validate : '', data : '', dataSource : '', path : []}
def do(context):
    print context
    global g_logger 
    g_logger = logging.getLogger("VerifyTexture")
    logging.basicConfig(level=logging.INFO, format="%(levelname)s %(name)s %(asctime)s: %(message)s")

    show_context(context)
    errNumber = 0
    if context['validate'] != g_mode_skip:
        try:
            for p in context['dirs']:
                dPath = os.path.join(context['data'], p)
                dsPath = os.path.join(context['data_source'],p)
                errNumber += verify(context['validate'], dPath, dsPath)
        except Exception, e:
            g_logger.error(e.message)
            sys.exit(e.message)
    if errNumber != 0:
        message = "Found " + str(errNumber) + " wrong texture size"
        g_logger.critical(message)
        print "##teamcity[buildStatus status='FAILURE' text='%s']" % message
        sys.exit(2)

    g_logger.info("Success!")
    return True

def main():
    context = get_input_context()
    do(context)

if __name__ == "__main__":
    main()