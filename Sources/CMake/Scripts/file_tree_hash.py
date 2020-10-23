#!/usr/bin/env python
import sys
import os
import hashlib
import timeit
import argparse

parser = argparse.ArgumentParser()
parser.add_argument( 'item_list', nargs='+' )
parser.add_argument( '--file_mode', action="store_false" )
args   = parser.parse_args()

count      = 0
array      = []
tree_hash  = hashlib.md5()

if( args.file_mode ) :
    for arg in args.item_list:
        for rootdir, dirs, files in os.walk( arg ):
            for file in files:   
                if file.endswith( ('.c','.cpp','.h','.hpp','.m','.mm','.ui','.qrc','.a','.so','.qrc','.lib')  ): 
                    array.append( os.path.relpath(( rootdir + file ), ( arg + '/..' ) )  ) 

    tree_hash.update( repr(array).encode('utf-8') )

else :
    tree_hash.update( repr(args.item_list).encode('utf-8') )

print tree_hash.hexdigest()
