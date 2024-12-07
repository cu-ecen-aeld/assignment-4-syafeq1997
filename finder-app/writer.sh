#!/bin/bash
#

writefile=$1
writestr=$2

if [ $# -ne 2 ]; then
    echo "no arguments supplied"
    exit 1
fi

dir_files=$(dirname $writefile)

if [ ! -d "$dir_files" ]; then
    echo "$dir_files directory does not exist"
    mkdir -p $dir_files
    if [ $? -ne 0 ]; then
       echo "Error: failed to create $dir_files"
       exit 1
    fi
fi

echo "$writestr" > "$writefile"
if [ $? -ne 0 ]; then
   echo "Error: failed to create $writefile"
   exit 1
fi

exit 0


