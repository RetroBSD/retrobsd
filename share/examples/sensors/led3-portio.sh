#!/bin/sh
#
# Example for RetroBSD on Olimex Duinomite board.
# RGB LED is connected to D7, D6 and D5 pins of Duinomite board.
# Use portio utility to control LED.
#

# Pins D5, D6 and D7 are connected to signals RE5, RE6 and RE7 of pic32 chip.
# Set up them as output.
portio -o e5-7
portio -c e5-7

while :
do
    portio -c e5 -s e7
    portio -c e7 -s e6
    portio -c e6 -s e5
done
