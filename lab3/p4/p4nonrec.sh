#!/bin/bash

dir="$1"
mod="$2"

for file in "$dir"/*.txt; do
    if [ "$mod" == "x" ] || [ "$mod" == "r" ] || [ "$mod" == "w" ]; then
        chmod +$mod $file
    fi
done

