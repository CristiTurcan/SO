#!/bin/bash

# email: ^([a-zA-Z0-9_\-\.]+)@([a-zA-Z0-9_\-\.]+)\.([a-zA-Z]{2,5})$
# ip?: ^((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[1-9])\.){3}(25[0-4]|2[0-4][0-9]|[01]?[0-9][0-9]?)$
# combinat: ^([a-zA-Z0-9_\-\.]+)@([a-zA-Z0-9_\-\.]+)\.([a-zA-Z]{2,5});((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[1-9])\.){3}(25[0-4]|2[0-4][0-9]|[01]?[0-9][0-9]?)$

input_file="$1"
output_file="$2"

while read -r line; do
    text=$(echo "$line" | grep -E "^([a-zA-Z0-9_\-\.]+)@([a-zA-Z0-9_\-\.]+)\.([a-zA-Z]{2,5});((25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[1-9])\.){3}(25[0-4]|2[0-4][0-9]|[01]?[0-9][0-9]?)$")
    
    if [ ! -z "$text" ] #check if text is not empty - if empty, text is not matched with regex
    then
        echo "OK" >> "$output_file"
    else
        echo "ERROR" >> "$output_file"
    fi

done < "$input_file"