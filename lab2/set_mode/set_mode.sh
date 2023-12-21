#!/bin/bash

dir="$1"

mod="$2"

if [ "$mod" != "x" ] && [ "$mod" != "w" ] && [ "$mod" != "r" ]; then
    echo "Invalid arguments"
    exit 1
fi

function permissions {
    cd $1
    for f in *.txt; do
        chmod +$mod $f
    done

    for d in *; do
        if [ -d "$d" ]; then
            permissions $d
        fi
    done

    cd ..
}

permissions dir
