#include <cassert>
#include <cstdio>
#include <cstdint>

#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include "ip.hh"
#include "util.hh"

using namespace std;

static uint32_t internet_checksum_pre( uint32_t start, const uint8_t * buf,
                                       uint16_t len )
{
  uint32_t sum = start;

  for ( ; len > 1; len -= 2, buf += 2 ) {
    sum += *(uint16_t *)buf;
  }

  if ( len > 0 ) {
    sum += *(uint8_t *)buf;
  }

  return sum;
}

/* Compute Internet Checksum for "len" bytes beginning at location "addr".
 * From: http://tools.ietf.org/html/rfc1071 */
uint16_t internet_checksum( const uint8_t * buf, uint16_t len )
{
  uint32_t sum = internet_checksum_pre( 0, buf, len );
  sum = ( sum & 0xffff ) + ( sum >> 16 );
  return ~sum;
}

/* Calculate the IPV4 header checksum given an IPV4 packet */
void calculate_ipv4_checksum( uint8_t * ipv4_pkt, uint16_t pkt_len )
{
  assert( pkt_len >= 20 );
  size_t hdr_len = ( ipv4_pkt[0] & 0x0f ) * 4;
  uint16_t * chk_field = reinterpret_cast<uint16_t *>( ipv4_pkt + 10 );

  *chk_field = 0; // zero out existing one
  *chk_field = internet_checksum( ipv4_pkt, hdr_len );
}

// XXX: I prefer this one which I (davidt) wrote, but GCC compiles it wrong! So
// to avoid annoying bug just taking one from internet.
// /* Calculate the IPV4-UDP header checksum given an IPV4-UDP packet */
// void calculate_ipv4_udp_checksum( uint8_t * ipv4_pkt, uint16_t pkt_len )
// {
//   assert( pkt_len >= 28 );
//
//   uint32_t chk;
//   uint16_t ip4_len = ( ipv4_pkt[0] & 0x0f ) * 4;
//   uint16_t udp_len = pkt_len - ip4_len;
//
//   uint32_t pseudo_hdr[3];
//   uint8_t *pseudo_hdr_8 = reinterpret_cast<uint8_t *>( pseudo_hdr );
//   uint16_t *pseudo_hdr_16 = reinterpret_cast<uint16_t *>( pseudo_hdr );
//
//   uint32_t *ipv4_32 = reinterpret_cast<uint32_t *>( ipv4_pkt );
//   uint16_t *ipv4_16 = reinterpret_cast<uint16_t *>( ipv4_pkt );
//
//   // zero old checksum
//   ipv4_16[ip4_len / 2 + 3] = 0;
//
//   // pseudo header
//   memset( pseudo_hdr, 0, 12 );
//   pseudo_hdr[0] = ipv4_32[3];          // source ip
//   pseudo_hdr[1] = ipv4_32[4];          // dest ip
//   pseudo_hdr_8[8] = 0;                 // zeroes
//   pseudo_hdr_8[9] = 0x11;              // protocol
//   pseudo_hdr_16[5] = htons( udp_len ); // udp length
//
//   // checksum pseudo header
//   chk = internet_checksum_pre( 0,  pseudo_hdr_8, 12 );
//   chk = internet_checksum_pre( chk, ipv4_pkt + ip4_len, pkt_len - ip4_len );
//   chk = ~(( chk & 0xffff ) + ( chk >> 16 ));
//
//   // add in new checksum
//   ipv4_16[ip4_len / 2 + 3] = chk;
// }

void calculate_ipv4_udp_checksum( uint8_t * ipv4_pkt, uint16_t pkt_len )
{
  struct iphdr * pIph = (struct iphdr *)ipv4_pkt;
  uint16_t ip4_len = ( ipv4_pkt[0] & 0x0f ) * 4;
  unsigned short * ipPayload = (unsigned short *)( ipv4_pkt + ip4_len );

  unsigned long sum = 0;
  struct udphdr * udphdrp = (struct udphdr *)( ipPayload );
  unsigned short udpLen = htons( udphdrp->len );
  // printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~udp len=%d\n", udpLen);
  // add the pseudo header
  // printf("add pseudo header\n");
  // the source ip
  sum += ( pIph->saddr >> 16 ) & 0xFFFF;
  sum += ( pIph->saddr ) & 0xFFFF;
  // the dest ip
  sum += ( pIph->daddr >> 16 ) & 0xFFFF;
  sum += ( pIph->daddr ) & 0xFFFF;
  // protocol and reserved: 17
  sum += htons( IPPROTO_UDP );
  // the length
  sum += udphdrp->len;

  // add the IP payload
  // printf("add ip payload\n");
  // initialize checksum to 0
  udphdrp->check = 0;
  while ( udpLen > 1 ) {
    sum += *ipPayload++;
    udpLen -= 2;
  }
  // if any bytes left, pad the bytes and add
  if ( udpLen > 0 ) {
    // printf("+++++++++++++++padding: %d\n", udpLen);
    sum += ( ( *ipPayload ) & htons( 0xFF00 ) );
  }
  // Fold sum to 16 bits: add carrier to result
  // printf("add carrier\n");
  while ( sum >> 16 ) {
    sum = ( sum & 0xffff ) + ( sum >> 16 );
  }
  // printf("one's complement\n");
  sum = ~sum;
  // set computation result
  udphdrp->check =
    ( (unsigned short)sum == 0x0000 ) ? 0xFFFF : (unsigned short)sum;
}

/* Calculate the IPV4-TCP header checksum given an IPV4-TCP packet */
void calculate_ipv4_tcp_checksum( uint8_t * ipv4_pkt, uint16_t pkt_len )
{
  struct iphdr * pIph = (struct iphdr *)ipv4_pkt;
  uint16_t ip4_len = ( ipv4_pkt[0] & 0x0f ) * 4;
  unsigned short * ipPayload = (unsigned short *)( ipv4_pkt + ip4_len );
  unsigned long sum = 0;
  unsigned short tcpLen = ntohs( pIph->tot_len ) - ( pIph->ihl << 2 );
  struct tcphdr * tcphdrp = (struct tcphdr *)( ipPayload );
  // add the pseudo header
  // the source ip
  sum += ( pIph->saddr >> 16 ) & 0xFFFF;
  sum += ( pIph->saddr ) & 0xFFFF;
  // the dest ip
  sum += ( pIph->daddr >> 16 ) & 0xFFFF;
  sum += ( pIph->daddr ) & 0xFFFF;
  // protocol and reserved: 6
  sum += htons( IPPROTO_TCP );
  // the length
  sum += htons( tcpLen );

  // add the IP payload
  // initialize checksum to 0
  tcphdrp->check = 0;
  while ( tcpLen > 1 ) {
    sum += *ipPayload++;
    tcpLen -= 2;
  }
  // if any bytes left, pad the bytes and add
  if ( tcpLen > 0 ) {
    // printf("+++++++++++padding, %d\n", tcpLen);
    sum += ( ( *ipPayload ) & htons( 0xFF00 ) );
  }
  // Fold 32-bit sum to 16 bits: add carrier to result
  while ( sum >> 16 ) {
    sum = ( sum & 0xffff ) + ( sum >> 16 );
  }
  sum = ~sum;
  // set computation result
  tcphdrp->check = (unsigned short)sum;
}

// XXX: I prefer this one which I (davidt) wrote, but GCC compiles it wrong! So
// to avoid annoying bug just taking one from internet.
// /* Calculate the IPV4-TCP header checksum given an IPV4-TCP packet */
// void calculate_ipv4_tcp_checksum( uint8_t * ipv4_pkt, uint16_t pkt_len )
// {
//   assert( pkt_len >= 40 );
//
//   uint32_t chk;
//   uint16_t ip4_len = ( ipv4_pkt[0] & 0x0f ) * 4;
//   uint16_t tcp_len = pkt_len - ip4_len;
//
//   uint32_t pseudo_hdr[3];
//   uint8_t *pseudo_hdr_8 = reinterpret_cast<uint8_t *>( pseudo_hdr );
//   uint16_t *pseudo_hdr_16 = reinterpret_cast<uint16_t *>( pseudo_hdr );
//
//   uint32_t *ipv4_32 = reinterpret_cast<uint32_t *>( ipv4_pkt );
//   uint16_t *ipv4_16 = reinterpret_cast<uint16_t *>( ipv4_pkt );
//
//   // zero old checksum
//   ipv4_16[ip4_len / 2 + 8] = 0;
//
//   // pseudo header
//   memset( pseudo_hdr, 0, 12 );
//   pseudo_hdr[0] = ipv4_32[3];          // source ip
//   pseudo_hdr[1] = ipv4_32[4];          // dest ip
//   pseudo_hdr_8[8] = 0;                 // zeroes
//   pseudo_hdr_8[9] = 0x6;               // protocol
//   pseudo_hdr_16[5] = htons( tcp_len ); // tcp length
//
//   // checksum pseudo header
//   chk = internet_checksum_pre( 0,  pseudo_hdr_8, 12 );
//   chk = internet_checksum_pre( chk, ipv4_pkt + ip4_len, pkt_len - ip4_len );
//   chk = ~(( chk & 0xffff ) + ( chk >> 16 ));
//
//   // add in new checksum
//   ipv4_16[ip4_len / 2 + 8] = chk;
// }

/* Print an IP packet header to stdout */
void print_ip_packet( const uint8_t * const ip_pkt, size_t size )
{
  if ( size < 20 ) {
    return;
  }

  char buf[1024];
  uint16_t udp_len;

  printf( "IP Version:\t\t%d\n", ip_pkt[0] >> 4 );
  int hl = ( ip_pkt[0] & 0x0f ) * 4;
  printf( "IP Header Length:\t%d bytes\n", hl );
  printf( "IP Total Length:\t%d bytes\n",
          ntohs( *(uint16_t *)( ip_pkt + 2 ) ) );
  printf( "IP ID:\t\t\t%d\n", ntohs( *(uint16_t *)( ip_pkt + 4 ) ) );
  printf( "IP Source Address\t%s\n",
          inet_ntoa( *(struct in_addr *)( ip_pkt + 12 ) ) );
  printf( "IP Destination Address\t%s\n",
          inet_ntoa( *(struct in_addr *)( ip_pkt + 16 ) ) );
  printf( "IP Checksum:\t\t%s\n", str_to_hex( ip_pkt + 10, 2 ).c_str() );
  const char * flag;
  if ( ip_pkt[6] & 0x40 ) {
    flag = "dont fragment (DF)";
  } else if ( ip_pkt[6] & 0x20 ) {
    flag = "more fragments (MF)";
  } else {
    flag = "none";
  }
  printf( "IP Fragment:\t\t%s\n", flag );
  uint16_t frag_offset = ntohs( *(uint16_t *)( ip_pkt + 6 ) ) & 0x1fff;
  printf( "IP Fragment Offset:\t%d\n", frag_offset );
  printf( "Payload type:\t\t" );
  switch ( ip_pkt[9] ) {
  case 1:
    printf( "ICMP\n" );
    break;
  case 6:
    printf( "TCP\n" );
    printf( "TCP Source Port:\t%d\n", ntohs( *(uint16_t *)( ip_pkt + hl ) ) );
    printf( "TCP Destination Port:\t%d\n",
            ntohs( *(uint16_t *)( ip_pkt + hl + 2 ) ) );
    printf( "TCP Checksum:\t\t%s\n",
            str_to_hex( ip_pkt + hl + 16, 2 ).c_str() );
    break;
  case 17:
    printf( "UDP\n" );
    printf( "UDP Source Port:\t%d\n", ntohs( *(uint16_t *)( ip_pkt + hl ) ) );
    printf( "UDP Destination Port:\t%d\n",
            ntohs( *(uint16_t *)( ip_pkt + hl + 2 ) ) );
    udp_len = ntohs( *(uint16_t *)( ip_pkt + hl + 4 ) );
    printf( "UDP Length:\t\t%d\n", udp_len );
    printf( "UDP Checksum:\t\t%s\n",
            str_to_hex( ip_pkt + hl + 6, 2 ).c_str() );
    if ( udp_len < 70 ) {
      memcpy( buf, ip_pkt + hl + 8, udp_len - 8 );
      buf[udp_len - 8] = '\0';
      printf( "UDP Payload:\t\t%s\n", buf );
    }
    break;
  default:
    printf( "%d\n", ip_pkt[9] );
  }
}

/* Build a raw UDP packet including IP header */
string build_udp_packet( uint16_t id, const Address & sip, const Address & dip,
                         uint16_t sport, uint16_t dport, const char * data )
{
  uint8_t pkt[IP_MAXPACKET];

  // Sizes
  size_t hdr_len = IP4_HDRLEN + UDP_HDRLEN;
  uint16_t data_len = min( strlen( data ), IP_MAXPACKET - hdr_len );
  size_t pkt_len = hdr_len + data_len;

  // Packet
  struct iphdr * iphdr = (struct iphdr *)pkt;
  struct udphdr * udphdr = (struct udphdr *)( pkt + IP4_HDRLEN );
  uint8_t * payload = pkt + hdr_len;

  // IPv4 header
  iphdr->version = 4;
  iphdr->ihl = 5;
  iphdr->tos = 0;
  iphdr->tot_len = 0;
  iphdr->tot_len = htons( pkt_len );
  iphdr->id = htons( id );
  iphdr->frag_off = 0;
  iphdr->ttl = DEFAULT_TTL;
  iphdr->protocol = IPPROTO_UDP;
  iphdr->check = 0;
  iphdr->saddr = sip.to_inaddr().s_addr;
  iphdr->daddr = dip.to_inaddr().s_addr;

  // IP Checksum
  iphdr->check = internet_checksum( pkt, IP4_HDRLEN );

  // UDP header
  udphdr->source = htons( sport );
  udphdr->dest = htons( dport );
  udphdr->len = htons( UDP_HDRLEN + data_len );
  udphdr->check = 0;

  // Payload
  memcpy( payload, data, data_len );

  return string( reinterpret_cast<char *>( pkt ), pkt_len );
}
