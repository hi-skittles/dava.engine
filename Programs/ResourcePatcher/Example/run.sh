#!/bin/bash
export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:./../Build/Products/Debug/
python MakeDiff.py -b Root_v1_g1.zip -n Root_v2_g2.zip -p 1.dat
