#ifndef _PACKET_H
#define _PACKET_H

/*
 * This is the core definition of a TMUX data packet.
 * The format is very simple and light.  A single byte
 * defines the type of the packet - this can be thought
 * of as the command.
 * The stream indicates which endpoint to communicate
 * with.  A stream of 0 is the "system" stream and is
 * used to control the connection.
 * A single byte defines the length of the data segment,
 * so up to 255 bytes can be transmitted in one packet.
 */
struct smux_packet {
    unsigned char type;
    unsigned char stream;
    unsigned char len;
    char data[255];
};

// Initiate a connection
#define C_CONNECT    0x01

// Connection established, here is the stream id
#define C_CONNECTOK  0x02
#define C_CONNECTFAIL 0x03

// Disconnect a stream
#define C_DISCONNECT 0x04

// Data transmission
#define C_DATA       0x05

// Proxy is terminating, close everything down.
#define C_QUIT       0x06

// If we don't see a PING every 10 seconds, the
// link must have died - abort.
#define C_PING       0x07
#define C_PONG       0x08

#define C_MAX       0x08

#endif
