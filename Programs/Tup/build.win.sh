#! /bin/sh -e

REPOSRC=https://github.com/gittup/tup.git
LOCALREPO=tup_win

# checkout or update from github repo
if [ ! -d $LOCALREPO/.git ]
then
    git clone $REPOSRC $LOCALREPO
    cd $LOCALREPO
    #git checkout v0.7.4
    git apply ../patch-1_add_davainclude.patch
    git apply ../patch-2_build_for_win.patch
else
    cd $LOCALREPO
fi

# check if mingw-w64 is installer
set +e
brew info *mingw*
HASMINGW=$?
set -e

if [ $HASMINGW -ne 0 ]; then
    echo "There is no mingw-w64 installation."
    echo 
    echo "See https://github.com/cosmo0920/homebrew-mingw_w64 for instaling instruction."
    echo
    read -n 1 -p "Try to install it now? (y/n):" answ

    if [ "$answ" = "y" ]; then
        echo "yes"
        # https://github.com/cosmo0920/homebrew-mingw_w64
        # for more info
        brew tap cosmo0920/mingw_w64
        brew tap homebrew/versions
        brew install-mingw-w64
    else
        echo "no"
    fi
fi

# build tup
sh ./bootstrap.sh
cd ..

# prepare tup executable
mkdir -p bin
cp ./$LOCALREPO/tup*.exe bin
cp ./$LOCALREPO/tup*.dll bin
