#!/bin/sh
#
# Example for RetroBSD on Olimex Duinomite board.
# RGB LED is connected to D7, D6 and D5 pins of Duinomite board.
# Use /dev/porte device to control LED.
#

# Pins D5, D6 and D7 are connected to signals RE5, RE6 and RE7 of pic32 chip.
# Set up them as output.
echo --------ooo----- > /dev/confe
echo --------000----- > /dev/porte

while :
do
    echo --------100----- > /dev/porte
    sleep 1
    echo --------010----- > /dev/porte
    sleep 1
    echo --------001----- > /dev/porte
    sleep 1
done
