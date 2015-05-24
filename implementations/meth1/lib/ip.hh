#include <cstddef>
#include <cstdint>

#include "address.hh"

#define DEFAULT_TTL 80

#define IP4_HDRLEN ( sizeof( struct iphdr ) )
#define UDP_HDRLEN ( sizeof( struct udphdr ) )
#define TCP_HDRLEN ( sizeof( struct tcphdr ) )

/* Compute Internet Checksum for "count" bytes beginning at location "addr".
 * From: http://tools.ietf.org/html/rfc1071 */
uint16_t internet_checksum( const uint8_t * buf, uint16_t len );

/* Calculate the IPV4 header checksum given an IPV4 packet */
int calculate_ipv4_checksum( uint8_t * ipv4_pkt, uint16_t pkt_len );

/* Calculate the IPV4-UDP header checksum given an IPV4-UDP packet */
int calculate_ipv4_udp_checksum( uint8_t * ipv4_pkt, uint16_t pkt_len );

/* Calculate the IPV4-TCP header checksum given an IPV4-TCP packet */
int calculate_ipv4_tcp_checksum( uint8_t * ipv4_pkt, uint16_t pkt_len );

/* Print an IP packet header to stdout */
void print_ip_packet( const uint8_t * const ip_pkt, size_t size );

/* Build a raw UDP packet including IP header */
std::string build_udp_packet( uint16_t id, const Address & sip,
                              const Address & dip, uint16_t sport,
                              uint16_t dport, const char * data );
