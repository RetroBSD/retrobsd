#!/bin/sh

# Set signal RD8 as input, RE0 as output.
echo -------i-------- > /dev/confd
echo ---------------o > /dev/confe
echo ---------------0 > /dev/porte

while :
do
    # Poll button (active low)
    read portd < /dev/portd
    case $portd in
    ???????0????????)
        # Switch relay on
        echo ---------------1 > /dev/porte
        ;;
    ???????1????????)
        # Switch relay off
        echo ---------------0 > /dev/porte
        ;;
    esac
done
