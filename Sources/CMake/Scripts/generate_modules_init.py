#!/usr/bin/env python
import os
import sys
import argparse

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('modules', nargs='*')
    parser.add_argument('-o', metavar='output', help='Output file', nargs=1)
    parser.add_argument('-r', metavar='root', help='Engine root path', nargs=1)
    parser.add_argument('--verbose', help='Be verbose', action="store_true")
    
    try:
        args = parser.parse_args()
    except SystemExit as e:
        print 'Error parse arguments !!! Argument were:', str(sys.argv)
        raise

    output = 'out.cpp'
    if args.o is not None:
        output = args.o[0]

    rootpath = '.'
    if args.r is not None:
        rootpath = args.r[0]

    # open output file
    outfile = open(output, 'w')

    outfile.write('#include "ModuleManager/ModuleManager.h"\n')
    outfile.write('#include "ModuleManager/IModule.h"\n\n')

    included_modules = []

    exitCode = 0
    for module in args.modules:
        module_header = '%s/%sModule.h' % (module, module)
        module_fw_path = 'Modules/%s/Sources/%s' % (module, module_header)

        has_header = False
        if os.path.isfile(rootpath + '/' + module_fw_path):
            has_header = True
            included_modules.append(module)
            outfile.write('#include "%s"\n' % module_header)
        else:
            exitCode = 1    # error of generating file
            print "Error: Cannot find %sModule.h in /Modules/%s" % (module, module)


        if args.verbose:
            print "Checking for %s = %s" % (module_fw_path, has_header)

    outfile.write('\n')
    outfile.write('namespace DAVA\n')
    outfile.write('{\n')
    outfile.write('Vector<IModule*> CreateModuleInstances(Engine* engine)\n')
    outfile.write('{\n')
    outfile.write('  Vector<IModule*> modules;\n')

    for module in included_modules:
        outfile.write('  modules.emplace_back(new %sModule(engine));\n' % module)

    outfile.write('  return modules;\n')

    outfile.write('}\n')
    outfile.write('} // namespace DAVA\n')

    if args.verbose:
        print "Initialization code was generated for: %s" % included_modules

    outfile.close()

    sys.exit(exitCode)

if __name__ == '__main__':
    main()
