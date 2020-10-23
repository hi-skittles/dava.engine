rem # Use this command-file to split image on channels
rem # call as: ./splitimage.sh image_pathname
rem # as result 4 files would be created in source image folder (r.png ... a.png)

ResourceEditor.exe -imagesplitter -split -file %1
