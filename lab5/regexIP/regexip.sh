#!/bin/bash

input_file="$1"

output_file="$2"

while read -r line || [ -n "$line" ]; do
    text=$(echo "$line" | grep -E "^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$")

    if [ ! -z "$text" ] #check if text is not empty
    then  
        echo $text >> $output_file
    fi 

done < $input_file


# merg IP-uri care incep cu 0. sau care se termina cu .255