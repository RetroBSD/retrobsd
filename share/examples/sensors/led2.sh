#!/bin/sh
#
# Example for RetroBSD on Olimex Duinomite board.
# RGB LED is connected to D1 and D0 pins of Duinomite board.
# Use /dev/porte device to control LED.
#

# Pins D0 and D1 are connected to signals RE0 and RE1 of pic32 chip.
# Set up them as output.
echo --------------oo > /dev/confe
echo --------------00 > /dev/porte

while :
do
    echo --------------10 > /dev/porte
    sleep 1
    echo --------------01 > /dev/porte
    sleep 1
done
