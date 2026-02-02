#include "networking.h"
#include "rtl8139.h"
#include "utils.h"
#include <stdint.h>
#include <byteswap.h>

void send_pkt(void *pkt, uint32_t packet_size, uint8_t src_mac[6], uint8_t dst_mac[6], uint16_t type)
{
   struct ethernet_header eth = {0};
   memcpy(eth.src, src_mac, 6);
   memcpy(eth.dest, dst_mac, 6);
   eth.type = type;
   rtl8139_send_header(&eth, sizeof(eth), pkt, packet_size);
}
void send_pkt_ip(void *pkt, uint32_t packet_size, uint8_t src_mac[6], uint8_t ip[4], uint16_t type, arp_table_entry *arp_table)
{
   struct ethernet_header eth = {0};
   for (int i = 0; i < 1024; i++)
   {
        if (arp_table[i].complete == 1)
        {
            uint8_t brk = 1;
            for (int x = 0; x < 4; x++)
            {
                if (ip[x] != arp_table[i].ip[x])
                {
                    brk = 0;
                    break;
                }
            }
            if (brk == 0)
                continue;
            memcpy(eth.dest, arp_table[i].mac, 6);
            break;
        }
        else 
        {
            return;
        }
   }
   memcpy(eth.src, src_mac, 6);
   eth.type = type;
   rtl8139_send_header(&eth, sizeof(eth), pkt, packet_size);
}

void explore_ip(uint8_t  ip[4], uint8_t src_mac[6], arp_table_entry *arp_table, char *pkt_recv)
{
    struct arp_packet pkt = {0};
    
    pkt.hardware_type = 0x1;
    pkt.protocol_type = 0x0800;
    pkt.hardware_len = 6;
    pkt.protocol_len = 4;
    pkt.opcode =  0x0001;
    memcpy(pkt.source_hw_address, src_mac, 6);
    pkt.source_protocol_address[0] = 10;
    pkt.source_protocol_address[1] = 0;
    pkt.source_protocol_address[2] = 2;
    pkt.source_protocol_address[3] = 15;
    memset(pkt.dest_hw_address, 0, 6);
    memcpy(pkt.dest_protocol_address, ip, 4);
    pkt.hardware_type = bswap_16(pkt.hardware_type);
    pkt.protocol_type = bswap_16(pkt.protocol_type);
    pkt.opcode = bswap_16(pkt.opcode);

    uint8_t dest[6] = {0};
    dest[0] = 0xFF;
    dest[1] = 0xFF;
    dest[2] = 0xFF;
    dest[3] = 0xFF;
    dest[4] = 0xFF;
    dest[5] = 0xFF;
    send_pkt(&pkt, sizeof(pkt), src_mac, dest, 0x0806);
    char *buf = malloc(64 + 20);

    int index = 0;
    while (index < 1024)
    {
        if (arp_table[index].complete == 0)
            break;
        index++;
    }

    while(1)
    {
        if (*pkt_recv == 1)
        {
            rtl8139_recv(buf, 64 + 20, pkt_recv);
            struct full_arp_packet pkt_response;
            memcpy(&pkt_response, &buf[4], 64);
            memcpy(&arp_table[index].mac, &pkt_response.arp.source_hw_address, 6);
            memcpy(&arp_table[index].ip, &ip, 4);
            arp_table[index].complete = 1;
            pkt_recv = 0;
            break;
        }
    }
}