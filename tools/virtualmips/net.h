#ifndef __NET_H__
#define __NET_H__

#include "utils.h"

/* Ethernet Constants */
#define N_ETH_ALEN  6
#define N_ETH_HLEN  sizeof(n_eth_hdr_t)

/* Ethernet Address */
typedef struct {
    m_uint8_t eth_addr_byte[N_ETH_ALEN];
} __attribute__ ((__packed__)) n_eth_addr_t;
/* Ethernet Header */
typedef struct {
    n_eth_addr_t daddr;         /* destination eth addr */
    n_eth_addr_t saddr;         /* source ether addr    */
    m_uint16_t type;            /* packet type ID field */
} __attribute__ ((__packed__)) n_eth_hdr_t;

/* Check for a broadcast/multicast ethernet address */
static inline int eth_addr_is_bcast (n_eth_addr_t * addr)
{
    return ((addr->eth_addr_byte[0] == 0xff)
        && (addr->eth_addr_byte[1] == 0xff)
        && (addr->eth_addr_byte[2] == 0xff)
        && (addr->eth_addr_byte[3] == 0xff)
        && (addr->eth_addr_byte[4] == 0xff)
        && (addr->eth_addr_byte[5] == 0xff));
}

static inline int eth_addr_is_mcast (n_eth_addr_t * addr)
{
    return ((!eth_addr_is_bcast (addr)) && (addr->eth_addr_byte[0] & 1));

}

#endif
