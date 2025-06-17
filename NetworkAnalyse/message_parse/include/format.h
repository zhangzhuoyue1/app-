#ifndef FORMAT_H
#define FORMAT_H

#include <queue>


/*
    @brief
    定义了数据包的格式
    1. 以太网头部
    2. IPv4头部
    3. TCP头部
    4. UDP头部
    5. ICMP头部
    6. ARP头部
    7. DNS头部
*/

typedef unsigned char u_char;     // 1 byte
typedef unsigned short u_short;   // 2 byte
typedef unsigned int u_int;       // 4 byte
typedef unsigned long u_long;     // 4 byte

#define ARP  "ARP"                 //
#define TCP  "TCP"                 //
#define UDP  "UDP"                 //
#define ICMP "ICMP"                //
#define DNS  "DNS"                 //

// 以太网头部
/*
+-------------------+-----------------+------+
|       6 byte      |     6 byte      |2 byte|
+-------------------+-----------------+------+
|destination address|  source address | type |
+-------------------+-----------------+------+
*/
typedef struct ether_header{   // 14 byte
    u_char  ether_des_host[6];  // 目的地址 [6 byte]
    u_char  ether_src_host[6];  // 源地址 [6 byte]
    u_short ether_type;        // type [2 byte]
}ETHER_HEADER;


// Ipv4 头部
/*
+-------+-----------+---------------+-------------------------+
| 4 bit |   4 bit   |    8 bit      |          16 bit         |
+-------+-----------+---------------+-------------------------+
|version|head length|  TOS/DS_byte  |        total length     |
+-------------------+--+---+---+----+-+-+-+-------------------+
|          identification           |R|D|M|    offset         |
+-------------------+---------------+-+-+-+-------------------+
|       ttl         |     protocal  |         checksum        |
+-------------------+---------------+-------------------------+
|                   source ip address                         |
+-------------------------------------------------------------+
|                 destination ip address                      |
+-------------------------------------------------------------+
*/
typedef struct ip_header{           // 20 byte
    u_char  versiosn_head_length;    //  版本 [4 bit] 头部长度 [4 bit]
    u_char  TOS;                     // 服务类型 [1 byte]
    u_short total_length;           // IP包总长度  [2 byte]
    u_short identification;         // 标识符 [2 byte]
    u_short flag_offset;            // 标志  [3 bit] 片偏移 [13 bit]
    u_char  ttl;                     // 生存时间  [1 byte]
     u_char protocol;                // 协议 [1字节]
    u_short checksum;               // 校验和 [2字节]
    u_int   src_addr;                 // 源地址 [4字节]
    u_int   des_addr;                 // 目的地址 [4字节]
} IP_HEADER;
// Tcp header
/*
+----------------------+---------------------+
|         16 bit       |       16 bit        |
+----------------------+---------------------+
|      source port     |  destination port   |
+----------------------+---------------------+
|              sequence number               |
+----------------------+---------------------+
|                 ack number                 |
+----+---------+-------+---------------------+
|head| reserve | flags |     window size     |
+----+---------+-------+---------------------+
|     checksum         |   urgent pointer    |
+----------------------+---------------------+
*/
typedef struct tcp_header{    // 20字节
    u_short src_port;         // 源端口 [2字节]
    u_short des_port;         // 目的端口 [2字节]
    u_int   sequence;           // 序列号 [4字节]
    u_int   ack;                // 确认号 [4字节]
    u_char  header_length;     // 头部长度 [4位]
    u_char  flags;             // 标志位 [6位]
    u_short window_size;      // 窗口大小 [2字节]
    u_short checksum;         // 校验和 [2字节]
    u_short urgent;           // 紧急指针 [2字节]
} TCP_HEADER;
// Udp header
/*
+---------------------+---------------------+
|        16 bit       |        16 bit       |
+---------------------+---------------------+
|    source port      |   destination port  |
+---------------------+---------------------+
| data package length |       checksum      |
+---------------------+---------------------+
*/
typedef struct udp_header{ // 8字节
    u_short src_port;      // 源端口 [2字节]
    u_short des_port;      // 目的端口 [2字节]
    u_short data_length;   // 数据长度 [2字节]
    u_short checksum;      // 校验和 [2字节]
} UDP_HEADER;

// Icmp header
/*
+---------------------+---------------------+
|  1 byte  |  1 byte  |        2 byte       |
+---------------------+---------------------+
|   type   |   code   |       checksum      |
+---------------------+---------------------+
|    identification   |       sequence      |
+---------------------+---------------------+
|                  option                   |
+-------------------------------------------+
*/
typedef struct icmp_header{         // 至少8字节
    u_char type;                    // 类型 [1字节]
    u_char code;                    // 代码 [1字节]
    u_short checksum;               // 校验和 [2字节]
    u_short identification;         // 标识符 [2字节]
    u_short sequence;               // 序列号 [2字节]
} ICMP_HEADER;

//Arp
/*
|<--------  ARP header  ------------>|
+------+--------+-----+------+-------+----------+---------+---------------+--------------+
|2 byte| 2 byte |1byte| 1byte|2 byte |  6 byte  | 4 byte  |     6 byte    |     4 byte   |
+------+--------+-----+------+-------+----------+---------+---------------+--------------+
| type |protocol|e_len|ip_len|op_type|source mac|source ip|destination mac|destination ip|
+------+--------+-----+------+-------+----------+---------+---------------+--------------+
*/
typedef struct arp_header{   // 28字节
    u_short hardware_type;   // 硬件类型 [2字节]
    u_short protocol_type;   // 协议类型 [2字节]
    u_char mac_length;       // MAC地址长度 [1字节]
    u_char ip_length;        // IP地址长度 [1字节]
    u_short op_code;         // 操作码 [2字节]

    u_char src_eth_addr[6];  // 源以太网地址 [6字节]
    u_char src_ip_addr[4];   // 源IP地址 [4字节]
    u_char des_eth_addr[6];  // 目的以太网地址 [6字节]
    u_char des_ip_addr[4];   // 目的IP地址 [4字节]
} ARP_HEADER;

// dns
/*
+--------------------------+---------------------------+
|           16 bit         |1b|4bit|1b|1b|1b|1b|3b|4bit|
+--------------------------+--+----+--+--+--+--+--+----+
|      identification      |QR| OP |AA|TC|RD|RA|..|Resp|
+--------------------------+--+----+--+--+--+--+--+----+
|         Question         |       Answer RRs          |
+--------------------------+---------------------------+
|     Authority RRs        |      Additional RRs       |
+--------------------------+---------------------------+
*/
typedef struct dns_header{  // 12 byte
    u_short identification; // Identification [2 byte]
    u_short flags;          // Flags [total 2 byte]
    u_short question;       // Question Number [2 byte]
    u_short answer;         // Answer RRs [2 byte]
    u_short authority;      // Authority RRs [2 byte]
    u_short additional;     // Additional RRs [2 byte]
}DNS_HEADER;

#endif // FORMAT_H


