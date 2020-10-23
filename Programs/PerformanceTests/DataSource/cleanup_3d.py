#!/usr/bin/env python
#
#  cleanup_3d.py
#  DAVA SDK
#
#  Created by Yury Danilov on 6/28/13.
#  Copyright (c) 2013 DAVA Consulting, LLC. All rights reserved.
#

import os;
import os.path;
import shutil;


#cleanup Data/3d
currentDir = os.getcwd(); 
dataDir =  os.path.realpath(currentDir + "/../Data/3d/")
if os.path.exists(dataDir):    
    print "delete " + dataDir
    shutil.rmtree(dataDir)
