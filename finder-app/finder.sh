#!/bin/sh

filesdir=$1
searchstr=$2


if [ $# -ne 2  ]; then
    echo "No arguments supplied"
    exit 1
fi

if [ ! -d "$filesdir" ]; then
    echo "$filesdir dir does not exist"
    exit 1
fi

list=$(find "$filesdir" -type f | wc -l)

match_count=$(grep -r "$searchstr" "$filesdir" 2>/dev/null | wc -l)

echo "The number of files are $list and the number of matching lines are $match_count."

exit 0

