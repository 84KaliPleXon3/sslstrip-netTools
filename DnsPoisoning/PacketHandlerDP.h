#pragma once

#include <windows.h>

#include "DnsPoisoning.h"
#include "NetworkStructs.h"
#include "LinkedListTargetSystems.h"

/*
 * Data types
 * 
 */

typedef struct
{
  u_char *pcapData;
  unsigned int pcapDataLen;
  PETHDR etherHdr;
  PIPHDR ipHdr;
  PTCPHDR tcpHdr;
  PUDPHDR udpHdr;
  int ipHdrLen;
  unsigned char srcIp[MAX_BUF_SIZE + 1];
  unsigned long srcIpBin;
  unsigned char dstIp[MAX_BUF_SIZE + 1];
  unsigned long dstIpBin;
  unsigned short dstPort;
  unsigned short srcPort;
  char proto[128];
  unsigned short pktLen;
  char suffix[MAX_BUF_SIZE + 1];
  char logMsg[MAX_BUF_SIZE + 1];
}
PACKET_INFO, *PPACKET_INFO;



/*
 * Function forward declarations
 *
 */
DWORD PacketHandlerDP(PSCANPARAMS lpParam);
BOOL ProcessData2Internet(PPACKET_INFO packetInfo, PSCANPARAMS scanParams);
BOOL ProcessData2GW(PPACKET_INFO packetInfo, PSCANPARAMS scanParams);
BOOL ProcessData2Victim(PPACKET_INFO packetInfo, PSYSNODE realDstSys, PSCANPARAMS scanParams);
void PrepareDataPacketStructure(const u_char *data, PPACKET_INFO packetInfo);
BOOL SendPacket(int maxTries, LPVOID writeHandle, u_char *data, unsigned int dataSize);
void DnsPoisoning_handler(u_char *param, const struct pcap_pkthdr *pktHeader, const u_char *data);
BOOL DP_ControlHandler(DWORD pControlType);
void CloseAllPcapHandles();
