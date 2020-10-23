#!/bin/bash
DIR="./../../../Libs/ImageMagick-6.6.9"
ARCH="ImageMagick-6.6.9-0.tar.gz"

if [ -d  $DIR ]
then
	echo "Magick exists"
else
	cd ./../../../Libs
	tar -xf $ARCH
	sh install_IM.sh
fi