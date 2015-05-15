#!/bin/sh
#
# Example for RetroBSD on Olimex Duinomite board.
# RGB LED is connected to D1 and D0 pins of Duinomite board.
# Use /dev/porte device to control LED.
#

# Pins D0 and D1 are connected to signals RE0 and RE1 of pic32 chip.
# Set up them as output.
portio -o e0 e1
portio -c e0 e1

while :
do
    portio -c e0 -s e1
    portio -c e1 -s e0
done
