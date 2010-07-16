#!/bin/bash
# Purpose: Install libCAENVME.so to your LD_LIBRARY_PATH
# Created by Exaos Lee <Exaos.Lee(at)gmail.com>, 2010-07-12

#######################################################################
# version comparison function that works for "." delimited version numbers (no alpha please)
# returns 0 for logically equal version numbers (2 == 2.0.0)
# returns 1 if the first param is later then the second param
# returns 2 if the second param is later than the first param
function vercmp()
{
    # just in case we get whole numbers, we append a .0 to the end of everything
    p1="${1}.0"
    p2="${2}.0"
    val1=`expr "$p1" : '\([0-9]*\)'`
    val2=`expr "$p2" : '\([0-9]*\)'`
    
    winner=0
    index=1
    while [[ ( -n "$val1" ||  -n "$val2" ) && "$winner" -eq "0" && "$index" -lt "10" ]]
    do
        # null is the same as 0 if we are still trying to match something
        if [ -z "$val1" ]
        then
            val1=0
        fi
        if [ -z "$val2" ]
        then
            val2=0
        fi
        
        # the greater number is always the winner at any equal tuple
        if [[ "$val1" -gt "$val2" ]]
        then
	    winner=1;
        fi
        if [[ "$val2" -gt "$val1" ]]
        then
	    winner=2;
        fi
	
        # make sure we end this thing on bad input
        p1_next_tuple_exists=`expr "$p1" : '[0-9]*\.'`
        if [[ "$p1_next_tuple_exists" -gt 0 ]]
        then
	    p1="${p1#*.}"
        else
	    p1=""
        fi
        p2_next_tuple_exists=`expr "$p2" : '[0-9]*\.'`
        if [[ "$p2_next_tuple_exists" -gt 0 ]]
        then
	    p2="${p2#*.}"
        else
	    p2=""
        fi
        
        let "index = $index + 1"
        val1="`expr "$p1" : '\([0-9]*\)'`"
        val2="`expr "$p2" : '\([0-9]*\)'`"
    done
    
    return $winner
}
#######################################################################

GLIBC_VERSION=`getconf GNU_LIBC_VERSION | awk '{print $2}'`
ARCH=`uname -m`
VERSION=2.11

vercmp ${GLIBC_VERSION} 2.3
if [[ $? = 1 ]]; then
    echo "Your glibc (${GLIBC_VERSION}) is newer than 2.3, so new version lib is used."
    LIBPATH=./lib
else
    echo "Your glibc (${GLIBC_VERSION}) is older than 2.3, so old version lib is used."
    LIBPATH=./lib/glibc-2.3
fi

if [[ ${ARCH} = 'x86_64' ]]; then
    LIBFILE=${LIBPATH}/x86_64/libCAENVME.so.${VERSION}
else
    LIBFILE=${LIBPATH}/libCAENVME.so.${VERSION}
fi

if [[ -z "$1" ]]; then
    echo "Destination path not specified. Where do your want to put the libCAENVME.so?"
    echo -n "Input path: "
    read res
    DESTDIR=$res
else
    DESTDIR=$1
fi

echo "Copying $LIBFILE to $DESTDIR ..."
cp -p $LIBFILE $DESTDIR
echo "Symbolic linking $DESTDIR/libCAENVME.so to $DESTDIR/libCAENVME.so.${VERSION}"
cd $DESTDIR
ln -s libCAENVME.so.${VERSION} libCAENVME.so

