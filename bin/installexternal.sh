#!/bin/bash

ENABLE_MPI="n"
ENABLE_DEBUG="n"
CLEANUP="n"
DOWNLOAD_ONLY="n"

TOPDIR=$(pwd)
EXTDIR=$(pwd)/external

installMETIS()
{
    cd $EXTDIR

    if [ ! -e metis-5.1.0.tar.gz ]; then
        wget http://glaros.dtc.umn.edu/gkhome/fetch/sw/metis/metis-5.1.0.tar.gz
    fi

    if  test "$DOWNLOAD_ONLY" == "y"; then
        return
    fi

    if  test "$CLEANUP" == "y"; then
        rm -rf metis-5.1.0
        return
    fi

    if ! test -e "metis-5.1.0"; then
        tar zxvf metis-5.1.0.tar.gz
    fi

    cd metis-5.1.0
    METISDIR=$(pwd)
    make config
    make

    cd $TOPDIR
}

installMultidomain()
{
    cd $TOPDIR

    if [ ! -e dune-multidomain ]; then
        git clone -b releases/2.0 git://github.com/smuething/dune-multidomain.git
    fi

    if  test "$DOWNLOAD_ONLY" == "y"; then
        return
    fi

    if  test "$CLEANUP" == "y"; then
        rm -rf dune-multidomain
        return
    fi

    # test for directory dune-common, dune-common-2.4 etc.
    if ! ls dune-common* &> /dev/null; then
        echo "You have to call installExternal for dune-multidomain from"
        echo "the same directory where dune-common is located. You"
        echo "cannot install it in the external folder."
        return
    fi

    cd $TOPDIR
}

installMultidomainGrid()
{
    cd $TOPDIR

    if [ ! -e dune-multidomaingrid ]; then
        git clone -b releases/2.3 git://github.com/smuething/dune-multidomaingrid.git
    fi

    if  test "$DOWNLOAD_ONLY" == "y"; then
        return
    fi

    if  test "$CLEANUP" == "y"; then
        rm -rf dune-multidomaingrid
        return
    fi

    # test for directory dune-common, dune-common-2.4 etc.
    if ! ls dune-common* &> /dev/null; then
        echo "You have to call installExternal for dune-multidomaingrid from"
        echo "the same directory where dune-common is located. You"
        echo "cannot install it in the external folder."
        return
    fi

    # apply patch for dune versions newer than 2.3
    cd dune-common
    VERSION=`git status | head -n 1 | awk '{ print $3 }'`
    if  [ "$VERSION" == "releases/2.4" ] || [ "$VERSION" == "master" ]; then
        echo "Applying patch"
        cd $TOPDIR/dune-multidomaingrid
        patch -p1 < $TOPDIR/dumux/patches/multidomaingrid-2.3.patch
    fi

    cd $TOPDIR
}

installUG()
{
    cd $EXTDIR

    if [ ! -e ug-3.12.1.tar.gz ]; then
        wget http://conan.iwr.uni-heidelberg.de/download/ug-3.12.1.tar.gz
    fi

    if  test "$DOWNLOAD_ONLY" == "y"; then
        return
    fi

    UG_VERSION="3.12.1"
    if  test "$CLEANUP" == "y"; then
        rm -rf ug-$UG_VERSION
        return
    fi

    if ! test -e "ug-$UG_VERSION"; then
        tar zxvf ug-$UG_VERSION.tar.gz
    fi

    cd ug-$UG_VERSION
    autoreconf -is
    OPTIM_FLAGS="-O3 -DNDEBUG -march=native -finline-functions -funroll-loops"
    # debug flags
    if test "$ENABLE_DEBUG" == "y"; then
        OPTIM_FLAGS="-O0 -g2"
    fi
    CFLAGS="$OPTIM_FLAGS"
    CXXFLAGS="$OPTIM_FLAGS -std=c++0x -fno-strict-aliasing"
    OPTS="--enable-dune --prefix=$PWD"

    if test "$ENABLE_MPI" == "y"; then
        OPTS="$OPTS --enable-parallel MPICC=$MPICXX"
    else
        OPTS="$OPTS --without-mpi"
    fi

    ./configure \
        CFLAGS="$CFLAGS" \
        CXXFLAGS="$CXXFLAGS" \
        $OPTS
    make
    make install

    cd $TOPDIR
}

installAll()
{
    installMETIS
    installMultidomain
    installMultidomainGrid
    installUG
}

createExternalDirectory()
{
    if [ ! -e $EXTDIR ]; then
        mkdir -v $EXTDIR
    fi
}

usage()
{
    echo "Usage: $0 [OPTIONS] PACKAGES"
    echo ""
    echo "Where PACKAGES is one or more of the following"
    echo "  all              Install everything and the kitchen sink."
    echo "  metis            Install the METIS graph partitioner."
    echo "  multidomain      Download dune-multidomain."
    echo "  multidomaingrid  Download dune-multidomaingrid."
    echo "  ug               Install the UG grid library."
    echo ""
    echo "The following options are recoginzed:"
    echo "    --parallel       Enable parallelization if available."
    echo "    --debug          Compile with debugging symbols and without optimization."
    echo "    --clean          Delete all files for the given packages."
    echo "    --download       Only download the packages."
}

SOMETHING_DONE="n"
for TMP in "$@"; do
    TMP=$(echo "$TMP" | tr "[:upper:]" "[:lower:]")
    case $TMP in
        "--debug")
            ENABLE_DEBUG="y"
            ;;
        "--download")
            DOWNLOAD_ONLY="y"
            ;;
        "--parallel")
            ENABLE_MPI="y"
            MPICC=$(which mpicc)
            MPICXX=$(which mpicxx)
            MPIF77=$(which mpif77)

            if test -f $(pwd)'/../dune-common/bin/mpi-config'; then 
                MPICONFIG=$(pwd)'/../dune-common/bin/mpi-config'
            else
                if test -f $(pwd)'/../dune-common-2.0/bin/mpi-config'
                then
                    MPICONFIG=$(pwd)'/../dune-common-2.0/bin/mpi-config'
                else 
                    echo "MPICONFIG not found!"
                    return
                fi
            fi 

            MPILIBS=$($MPICONFIG --libs)
            MPILIBDIR=$(echo $MPILIBS | sed "s/.*-L\([^[:blank:]]*\).*/\1/")

            # consistency check 
            if test "$ENABLE_MPI" == "y" -a -z "$MPICXX"; then 
                echo ""
                echo "Compiler mpicxx not found although ENABLE_MPI is set in this script!"
                echo "Please make sure that your MPI environment is set up or that you turn it off."
                echo "The shell command mpi-selector may help you to select an installed mpi-version."
                echo "Reinitilize your PATH variable after using it (e.g. logout and login again)."
                echo "Due to this error this script stops further building now."
                echo ""

                exit -1
            fi 

            ;;
        "--clean")
            CLEANUP="y"
            ;;
        all)
            SOMETHING_DONE="y"
            createExternalDirectory
            installAll
            ;;
        metis)
            SOMETHING_DONE="y"
            createExternalDirectory
            installMETIS
            ;;
        multidomain|dune-multidomain)
            SOMETHING_DONE="y"
            installMultidomain
            ;;
        multidomaingrid|dune-multidomaingrid)
            SOMETHING_DONE="y"
            installMultidomainGrid
            ;;
        ug)
            SOMETHING_DONE="y"
            createExternalDirectory
            installUG
            ;;
        *)
            usage
            exit 1
    esac
    cd $TOPDIR
done

if test "$SOMETHING_DONE" != "y"; then
    usage
    exit 1;
fi
