#!/bin/bash

ch="$1"
counter=0

if [ "$#" -ne 1 ]; then
    printf 'ERROR!Only one argument: bash script.sh <ch>\n'
    exit 1
fi

while read -r line
do
    text=$(echo "$line" | grep -E "^[A-Z][a-zA-Z0-9 , ]+\.$" | grep -v "si, " | grep -v "n[pb]")

    if [ ! -z "$text" ] && [[ $text =~ $ch ]]; then
        ((counter++))
    fi
done

echo "$counter "