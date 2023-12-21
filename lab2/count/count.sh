#!/bin/bash

dir="$1"

out="$2"

for file in "$dir"/*.txt; do
    echo -n `basename $file` >> $out
    wc -c < $file >> $out
done

echo -n "Total: " >> $out
cat "$dir"/*.txt | wc -c >> $out