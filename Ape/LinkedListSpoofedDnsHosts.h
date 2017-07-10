#pragma once

#define MAX_NODE_COUNT 1024 


typedef struct 
{
  unsigned char HostName[MAX_BUF_SIZE + 1];
  unsigned char SpoofedIP[MAX_IP_LEN + 1];
  void *DNSResponsePacket;
} HOSTDATA;


typedef struct 
{
  HOSTDATA sData;
  int first;
  struct HOSTNODE *prev;
  struct HOSTNODE *next;
} HOSTNODE, *PHOSTNODE, **PPHOSTNODE;




PHOSTNODE InitHostsList();
void AddSpoofedIpToList(PPHOSTNODE pHostNodes, unsigned char *pHostName, unsigned char *pSpoofedIP);
PHOSTNODE GetNodeByHostname(PHOSTNODE sysNodesParam, unsigned char *hostnameParam);
