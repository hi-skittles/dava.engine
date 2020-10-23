#! /bin/sh -e

REPOSRC=https://github.com/gittup/tup.git
LOCALREPO=tup_macos

# checkout or update from github repo
if [ ! -d $LOCALREPO/.git ]; then
    git clone $REPOSRC $LOCALREPO
    cd $LOCALREPO
    #git checkout v0.7.4
    git apply ../patch-1_add_davainclude.patch
else
    cd $LOCALREPO
fi

# build tup
sh ./bootstrap.sh
cd ..

# prepare tup executable
mkdir -p bin
cp ./$LOCALREPO/tup bin

# override libosxfuse tup dependency from system to local
#cp /usr/local/lib/libosxfuse_i64.2.dylib bin/libosxfuse.dylib
#install_name_tool -change /usr/local/lib/libosxfuse_i64.2.dylib libosxfuse.dylib bin/tup
#chmod a-x bin/libosxfuse.dylib
