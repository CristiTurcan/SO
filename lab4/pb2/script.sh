#!/bin/bash

# url: ^https?:\/\/(?:www\.)?[-a-zA-Z0-9@:%._\+~#=]*\.(com|eu|ro)$
# mac: ^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$
# combinat: ^https?:\/\/(?:www\.)?[-a-zA-Z0-9@:%._\+~#=]*\.(com|eu|ro)(?:;([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2}))?$

input_file="$1"
output_file="$2"

while read -r line; do
    text=$(echo "$line" | grep -E "^https?:\/\/(?:www\.)?[-a-zA-Z0-9@:%._\+~#=]*\.(com|eu|ro)(?:;([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2}))?$")
    
    if [ ! -z "$text" ] #check if text is not empty - if empty, text is not matched with regex
    then
        echo "OK" >> "$output_file"
    else
        echo "ERROR" >> "$output_file"
    fi

done < "$input_file"