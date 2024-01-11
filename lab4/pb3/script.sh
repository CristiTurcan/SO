#!/bin/bash

#bash script.sh <adresa mac>

# Adresa MAC initiala
mac_address="$1"

# Transformare adresa MAC
formatted_mac=$(echo $mac_address | sed 's/../&:/g; s/:$//')

# Afisare rezultat
echo "Adresa MAC formatata: $formatted_mac"
