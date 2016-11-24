#ifndef __ARPPOISONINGENGINE__
#define __ARPPOISONINGENGINE__

#include <windows.h>
#include <stdlib.h>
#include <stdint.h>
#include "NetBase.h"


#define APESNIFFER_VERSION "0.1"

//#define WITH_HTTP_INJECTION 0
//#define WITH_DNS_SPOOFER 0
//#define WITH_FIREWALL 0
//#define WITH_SNIFFER 0

#define DEBUG_LEVEL 0

#define DBG_OFF    0
#define DBG_INFO   1
#define DBG_LOW    2
#define DBG_MEDIUM 3
#define DBG_HIGH   4
#define DBG_ALERT  5
#define DBG_ERROR  5


//#define FILE_HOST_TARGETS ".targethosts"
//#define FILE_FIREWALL_RULES1 ".fwrules"
//#define FILE_FIREWALL_RULES2 "bin\\.fwrules"
//#define FILE_DNS_POISONING ".dnshosts"
//#define FILE_HTTPINJECTION_RULES1 ".injecturls"
//#define FILE_HTTPINJECTION_RULES2 "bin\\.injecturls"
//#define FILE_UNPOISON ".UNPOISON"
#define DBG_LOGFILE "c:\\debug.log"
//#define HOSTS_FILE "hosts.txt"


#define TCP_MAX_ACTIVITY 10
#define MAX_ID_LEN 128
#define MAX_PAYLOAD 1460
#define MAX_CONNECTION_VOLUME 4096
#define MAX_CONNECTION_COUNT 1024

#define MAX_SYSTEMS_COUNT 1024
#define MAX_ARP_SCAN_ROUNDS 5

#define MAX_BUF_SIZE 1024
#define MAX_PACKET_SIZE 512


#define PCAP_READTIMEOUT 1

#define SLEEP_BETWEEN_ARPS 50
#define SLEEP_BETWEEN_REPOISONING 5000 // 10 secs
#define SLEEP_BETWEEN_ARPSCANS 120 * 1000 // 120 secs

#define snprintf _snprintf

#define OK 0
#define NOK 1


/*
* Type definitions
*
*/
typedef struct
{
  unsigned char sysIpStr[MAX_IP_LEN + 1];
  unsigned char sysIpBin[BIN_IP_LEN];
  unsigned char sysMacBin[BIN_MAC_LEN];
  unsigned char srcIpStr[MAX_IP_LEN + 1];
  unsigned char dstIpStr[MAX_IP_LEN + 1];
  unsigned short srcPort;
  unsigned short dstPort;
} SYSTEMNODE, *PSYSTEMNODE, **PPSYSTEMNODE;



typedef struct SCANPARAMS
{
  unsigned char IFCName[MAX_BUF_SIZE + 1];
  unsigned char IFCAlias[MAX_BUF_SIZE + 1];
  unsigned char IFCDescr[MAX_BUF_SIZE + 1];
  int Index;
  unsigned char GWIP[BIN_IP_LEN];
  unsigned char GWIPStr[MAX_IP_LEN];
  unsigned char GWMAC[BIN_MAC_LEN];
  unsigned char GWMACStr[MAX_MAC_LEN];
  unsigned char StartIP[BIN_IP_LEN];
  unsigned long StartIPNum;
  unsigned char StopIP[BIN_IP_LEN];
  unsigned long StopIPNum;
  unsigned char LocalIP[BIN_IP_LEN];
  unsigned char LocalIPStr[MAX_IP_LEN];
  unsigned char LocalMAC[BIN_MAC_LEN];
  unsigned char LocalMACStr[MAX_MAC_LEN];
  unsigned char *PCAPPattern;
  unsigned char OutputPipeName[MAX_BUF_SIZE + 1];
  HANDLE PipeHandle;
  void *IfcReadHandle;  // HACK! because of header hell :/
  void *IfcWriteHandle; // HACK! because of header hell :/
} SCANPARAMS, *PSCANPARAMS;



/*
 * Function forward declarations.
 *
 */
void stringify(unsigned char *pInput, int pInputLen, unsigned char *pOutput);
//BOOL APE_ControlHandler(DWORD lala);
//void writeDepoisoningFile(void);
//void startUnpoisoningProcess();
void ExecCommand(char *pCmd);
void TerminateThreads();
void LogMsg(int pPriority, char *pMsg, ...);
void PrintConfig(SCANPARAMS pScanParams);
void PrintTimestamp(char *pTitle);
int UserIsAdmin();
void adminCheck(char *pProgName);
void PrintUsage(char *pAppName);
//void ParseTargetHostsConfigFile(char *pTargetsFile);
//void parseDNSConfigFile();

#endif

