#!/bin/bash

dir="$1"

output_file="$2"

count=0
count2=0

for entry in "$dir"/*; do
    if [ -f $entry ]
    then
        res=`cat $entry | grep -E "^[A-Z][a-zA-Z0-9 , ]+\.$" | grep -v "si, " | grep -v "n[pb]"`

        if [ ! -z "$res" ]
        then
            echo $res >> $output_file
        fi
    elif [ -L $entry ]
    then
        conut=expr $count + 1
    elif [ -d $entry ]
    then
        count2=bash $0 $entry $output_file
        count=expr $count + $count2
    fi
done

echo $count >> $output_file

# nu cred ca merge recursivitatea