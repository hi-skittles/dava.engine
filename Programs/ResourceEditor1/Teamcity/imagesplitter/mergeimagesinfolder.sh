#!/bin/bash

# Use this command-file to merge 4 channel images to one
# call as: ./mergeimagesinfolder.sh sourceimages_folder
# as result generated image would be created in sourceimages_folder (merged.png)

ResourceEditor.app/Contents/MacOS/ResourceEditor -imagesplitter -merge -folder $1
