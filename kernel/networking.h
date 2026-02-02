#ifndef NETWORKING_H
#define NETWORKING_H
#include <stdint.h>
#include "rtl8139.h"

typedef struct
{
    uint8_t  ip[4];
    uint8_t mac[6];
    uint8_t complete;
} arp_table_entry;

struct ethernet_header
{
    uint8_t dest[6];
    uint8_t src[6];
    uint16_t type;
}__attribute__((packed));

struct arp_packet
{
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t  hardware_len;
    uint8_t  protocol_len;
    uint16_t opcode; // ARP Operation Code
    uint8_t  source_hw_address[6];
    uint8_t  source_protocol_address[4];
    uint8_t  dest_hw_address[6];
    uint8_t  dest_protocol_address[4];
}__attribute__((packed));

struct full_arp_packet
{
    struct ethernet_header eth;
    struct arp_packet arp;
} __attribute__((packed));


void send_pkt(void *pkt, uint32_t packet_size, uint8_t src_mac[6], uint8_t dst_mac[6], uint16_t type);
void send_pkt_ip(void *pkt, uint32_t packet_size, uint8_t src_mac[6], uint8_t ip[4], uint16_t type, arp_table_entry *arp_table);
void explore_ip(uint8_t  ip[4], uint8_t src_mac[6], arp_table_entry *arp_table, char *pkt_recv);
#endif