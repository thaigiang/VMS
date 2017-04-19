#include "TCPIP Stack/TCPIP.h"
#include <p18cxxx.h>
#include "stdlib.h"
#include "uart.h"
#include "Main.h"
#include "typedef.h"
#include "gprs.h"
#include "transfer.h"
#include "flash.h"


#pragma config WDT=ON, FOSC2=ON, FOSC=HSPLL, ETHLED=ON, XINST=OFF, WDTPS=16384		// Enable watchdog timer, period ~ 65 second

//====================================
//#define USING_SIM_IMEI 1

#define SERVER_TAG			0
#define DEVICE_TAG 			1
#define ETHERNET_TAG 		2


#define CONFIG_ADDRESS		0x1D7C0//0x1F400
#define USER_ADDRESS		0x1D5C0//0x1F200


#define SEND_REQUEST_UPDATE		0xEF
#define CMD_REBOOT_REQUEST		0xAB
#define SEND_WARNING_HEADER		0xAB
#define CMD_REQUEST_RECONNECT	0x1A

#define RETRY_CONNECT			5
#define NW_TIMEOUT				60

static ROM BYTE SerializedMACAddress[6] = {MY_DEFAULT_MAC_BYTE1, MY_DEFAULT_MAC_BYTE2, MY_DEFAULT_MAC_BYTE3, MY_DEFAULT_MAC_BYTE4, MY_DEFAULT_MAC_BYTE5, MY_DEFAULT_MAC_BYTE6};
// Global variables
// Declare AppConfig structure and some other supporting stack variables
APP_CONFIG AppConfig;
HW_CONFIG hwConfig;
WN_CONFIG wnConfig;
tREALTIME rts = {0x00,0x52,0x15,0x06,0x23,0x08,0x13};
BYTE rcvData[MAX_DATA_PACKAGE];
unsigned int rcvCount = 0;
extern BYTE recvBuffer[512];
u8 simResCnt = 0;
bool fSendState = false;
bool fReceive = false;
bool fSendData = false;
bool fConfig = false;
bool fFactoryRST = false;
bool fConfigSIM = false;
bool fIshwConfigRequested = false;
unsigned int timeOut = 0;
NETWORK_STATUS nwState = NETWORK_LOST;
SIM_STATUS simStatus = SIM_OFF;
char serverIP[16] = " ";
char opPort[6] = " ";
char reportPort[6] = " ";
BYTE username[64] = "admin\0";
BYTE password[64] = "letihn0123\0";
u32 resetCounter = 0;
WORD mainVersion = 0;
WORD dplVersion = 0;
IP_ADDR IPServer;

extern SOCKET bsdOperateSocket;
extern SOCKET bsdEventSocket;
extern bool fSendCmd;
extern bool fGetIMEI;
extern BSD_STATE  BSDClientState;

//====================================
//Control interrupt 
void HighISR(void);
void LowISR(void);

// Functions prototype
void init_board(void);
void transferData(u8 data[],u16 length);
//void sendIMEI(u8 con_num);
void factoryReset(void);
bool start_connect(void);
static void IP2String(IP_ADDR ip,char* s);

void m_memcpy(u8* dest,u8* src,u16 length)
{
	u16 i = 0;
	for(i = 0;i<length;i++)
	{
		*dest = *src;
		dest ++;
		src ++;
	}
}

//====================================
#pragma code lowVector=0x18
void LowVector(void)
{
	_asm goto LowISR _endasm
}

#pragma code high_vector=0x08 
void HighVector(void) 
{ 
  _asm GOTO HighISR _endasm 
} 

#pragma code
#pragma interruptlow LowISR
void LowISR(void)
{
    TickUpdate();

	// Timer0 interrupt
	if(INTCONbits.TMR0IF == 1)
	{
		// Clear interrupt flag
		INTCONbits.TMR0IF = 0;
	}		
}

#pragma interrupt HighISR
void HighISR()
{
	// UART interrupt
	if(PIR1bits.RC1IF)
	{	
		unsigned char c;
		
		c = uart1_getc();
		
		if(rcvCount<MAX_DATA_PACKAGE)
		{
			rcvData[rcvCount] = c;
			if((rcvCount==0)&&(rcvData[0]!=DEVICE_TAG)&&(rcvData[0]!=SEND_WARNING_HEADER))
			{
				rcvData[0] = 0x00;
				rcvCount = 0;
			}
			else
				rcvCount++;

			if(rcvData[rcvCount-1] == '\n')
			{
				if(rcvData[rcvCount-2] == '\r')
				{
					fSendData = true;
				}
			}
		}
		else
			fSendData = true;
		//Clear receive interrupt flag
		PIR1bits.RC1IF = 0;
	}

	if(PIR3bits.RC2IF)
	{
		unsigned char c;

		c = uart2_getc();
		if(fSendCmd)
		{		
			if(simResCnt<512)
			{
				recvBuffer[simResCnt++] = c;				

				if(recvBuffer[simResCnt-1] == '\n')
				{
					if(recvBuffer[simResCnt-2] == '\r')
					{
						fReceive = true;
					}
				}
			}
			else
				fReceive = true;
		}
		else
			//uart1_putc(c);
			TXREG1 = c;
		
		//Clear receive interrupt flag
		PIR3bits.RC2IF = 0;
	}
}

//====================================
void main(void)
{
	static DWORD t = 0;
	unsigned long i = 0;
	unsigned long count = 0;
	bool connectionResult = false;
	bool fflag = false;
	
	for(i = 0;i<MAX_DATA_PACKAGE;i++)
		rcvData[i] = 0;	

	fConfig = true;
	init_board();		
	
	// Send dummy data
	for(i = 0;i< 32; i ++)
	{
		uart1_putc(0xFF);
	}
	
	TickInit();
	
	#if defined(STACK_USE_MPFS) || defined(STACK_USE_MPFS2)
	if(fConfig)
		MPFSInit();
	#endif
//	InitWNConfig();

	// Initialize core stack layers (MAC, ARP, TCP, UDP) and
	// application modules (HTTP, SNMP, etc.)
    StackInit();
	RCONbits.NOT_POR = 0;
	
	while(1)
	{	
		ClrWdt();

		//check test mode enable
		if(!STM32_RST_IN)
		{
			fflag = true;
			resetCounter ++;
			
		}
		else if (STM32_RST_IN && fflag)
		{
			fflag = false;
			if (resetCounter > 2000)	
			{
				// do test mode
				u8 buff1[2];
				buff1[0] = ETHERNET_TAG;
				buff1[1] = ETHERNET_SCAN_LED;
				transferData(buff1,2);
			}
			
			resetCounter = 0;
		}
	

		if(fConfigSIM)
		{
			init_uart2();
			SIM_PowerControl(SIM_POWER_ON);			
			configSIM(AppConfig.simType);
			SIM_PowerControl(SIM_POWER_OFF);	
			deinit_uart2();
			fConfigSIM = false;
		}
		if(fGetIMEI)
		{
			#ifdef USING_SIM_IMEI
			init_uart2();
			SIM_PowerControl(SIM_POWER_ON);
			SIM_GetIMEI(hwConfig.imei);
			SIM_PowerControl(SIM_POWER_OFF);
			deinit_uart2();
			#else
			GetIMEIFromMain(hwConfig.imei);
			#endif
			SaveHWConfig();
			SetHWConfig();
			fGetIMEI = false;
		}
		// Check need to send data to server or not?
		if(fSendData)
		{			
			if(rcvData[1] == CMD_REBOOT_REQUEST)
			{
				if(rcvData[2] == SEND_REQUEST_UPDATE)
				{
					Reset();
					//CPU_Reset();
				}
			}
			else if((rcvData[1] == CMD_REQUEST_RECONNECT)&&(rcvData[2] == CMD_REQUEST_RECONNECT))
			{	
				// Close all current connection
				if(nwState == NETWORK_ETHERNET)
				{
					BSDClientState = BSD_CLOSE;
				}
				else if(nwState == NETWORK_GPRS)
				{
					SIM_SendCmd((rom char *)"AT+CIPSHUT",(rom char *)"OK",(rom char *)"ERROR");
					timeOut = NW_TIMEOUT - 1;
				}
				// Change network state to reconnect
				nwState = NETWORK_LOST;
			}
			else
			{				
				// Check current network type (Ethernet or GPRS) to send data
				if(nwState == NETWORK_ETHERNET)
				{
					if(rcvData[0] == SEND_WARNING_HEADER)
						i = send(bsdEventSocket,&rcvData[1],rcvCount-3, 0);
					else if(rcvData[0] == DEVICE_TAG)
						i = send(bsdOperateSocket,rcvData,rcvCount-2, 0);					
				}
				else if(nwState == NETWORK_GPRS)
				{
					if(rcvData[0] == SEND_WARNING_HEADER)
						GPRS_SendData(1,&rcvData[1],rcvCount-3);
					else if(rcvData[0] == DEVICE_TAG)
						GPRS_SendData(0,rcvData,rcvCount-2);
				}
			}

			// Clear global variables
			for(i = 0;i<rcvCount;i++)
				rcvData[i] = 0;	
			rcvCount = 0;

			// Clear flag
			fSendData = false;
		}
	
        if(TickGet() - t >= TICK_SECOND/2ul)
        {
            t = TickGet();
			// If ethernet connection lost, try to connect through GPRS
			//if((nwState == NETWORK_LOST) && (fSendState == false))
			if((fConfig)&&(nwState == NETWORK_LOST))
			{
				timeOut++;
				if(timeOut >= NW_TIMEOUT)
				{		
					if(AppConfig.GPRSEnable)
					{
						// Power on module SIM
						init_uart2();
						if(simStatus == SIM_READY)
							SIM_SendCmd((rom char *)"AT+CFUN=1",(rom char *)"OK",(rom char *)"ERROR");
						else
							simStatus = SIM_PowerControl(SIM_POWER_ON);					
						// If module sim not ready, change network state to NETWORK_LOST
						if(simStatus == SIM_READY)
						{												
							// If module sim ready, try connect to server
							i = 0;
							connectionResult = false;
							while((!connectionResult) && (i < RETRY_CONNECT))
							{
								connectionResult = start_connect();
								i++;
							}
							if(!connectionResult)
							{
								SIM_SendCmd((rom char *)"AT+CFUN=0",(rom char *)"OK",(rom char *)"ERROR");
								deinit_uart2();
							}
						}
						else
						{
							SIM_SendCmd((rom char *)"AT+CFUN=0",(rom char *)"OK",(rom char *)"ERROR");
							deinit_uart2();
						}
					}
					// Send network state to server
					{
						u8 buff[2];
						buff[0] = ETHERNET_TAG;
						buff[1] = nwState;
						transferData(buff,2);
						fSendState = true;
					}
					timeOut = 0;
				}
			}
        }

		if((fSendState == false) &&(nwState == NETWORK_ETHERNET))
		{
			if(AppConfig.GPRSEnable)
			{
				simStatus = SIM_PowerControl(SIM_POWER_OFF);
				deinit_uart2();
			}
			{
				u8 buff[2];
				buff[0] = ETHERNET_TAG;
				buff[1] = nwState;
				transferData(buff,2);
				fSendState = true;
			}
		}

		// This task performs normal stack task including checking
        // for incoming packet, type of packet and calling
        // appropriate stack entity to process it.
        StackTask();
        
        // This tasks invokes each of the core stack application tasks
        StackApplications();
		
		#if defined(STACK_USE_BERKELEY_API)
		if(fConfig)
			BerkeleyTCPClient();
		#endif
	}
}
//====================================
void GetVersion(void)
{
	u8 buff[10];
	u8 i = 0;

	// Disable UART interrupt
	PIE1bits.RC1IE = 0;
	buff[0] = ETHERNET_TAG;
	buff[1] = ETHERNET_GET_VERSION;
	transferData(buff,2);
	i = 0;
	do
	{
		while(!PIR1bits.RC1IF) {};
		buff[i] = uart1_getc();
		i++;
	} while(buff[i-1]!='\n' && buff[i-2]!='\r');
	memcpy((void*)&dplVersion,(void*)&buff[1],2);
	memcpy((void*)&mainVersion,(void*)&buff[3],2);
	// Enable UART interrupt
	PIE1bits.RC1IE = 1;
}


//====================================
void GetDateTime(void)
{
	u8 buff[10];
	u8 i = 0;

	// Disable UART interrupt
	PIE1bits.RC1IE = 0;
	buff[0] = ETHERNET_TAG;
	buff[1] = ETHERNET_GET_TIME;
	transferData(buff,2);
	i = 0;
	do
	{
		while(!PIR1bits.RC1IF) {};
		buff[i] = uart1_getc();
		i++;
	} while(buff[i-1]!='\n' && buff[i-2]!='\r');
	memcpy((void*)&rts,(void*)&buff[1],sizeof(rts));
	// Enable UART interrupt
	PIE1bits.RC1IE = 1;
}

//====================================
void SetDateTime(void)
{
	u8 buff[10];

	buff[0] = ETHERNET_TAG;
	buff[1] = ETHERNET_SET_TIME;
	memcpy((void*)&buff[2],(void*)&rts,sizeof(rts));
	transferData(buff,sizeof(rts)+2);		
}


//====================================
void GetIMEIFromMain(unsigned char *imei)
{
	u8 buff[25];
	u8 i = 0;
	// Disable UART interrupt
	PIE1bits.RC1IE = 0;
	buff[0] = ETHERNET_TAG;
	buff[1] = ETHERNET_GET_IMEI;
	transferData(buff,2);
	i = 0;
	do
	{
		while(!PIR1bits.RC1IF) {};
		buff[i] = uart1_getc();
		i++;
	} while(buff[i-1]!='\n' && buff[i-2]!='\r');
	memcpy(imei,(const void *)&buff[1],15);
	// Enable UART interrupt
	PIE1bits.RC1IE = 1;
}



//====================================
void GetHWConfig(void)
{
	u8 buff[25];
	u8 i = 0;

	// Disable UART interrupt
	PIE1bits.RC1IE = 0;
	buff[0] = ETHERNET_TAG;
	buff[1] = ETHERNET_GET_CONFIG;
	transferData(buff,2);
	i = 0;
	do
	{
		while(!PIR1bits.RC1IF) {};
		buff[i] = uart1_getc();
		i++;
	} while(buff[i-1]!='\n' && buff[i-2]!='\r');
	memcpy((void*)&hwConfig,(void*)&buff[1],sizeof(hwConfig));
	// Enable UART interrupt
	PIE1bits.RC1IE = 1;
}

//====================================
void SetHWConfig(void)
{
	u8 buff[2];

	buff[0] = ETHERNET_TAG;
	buff[1] = ETHERNET_SET_CONFIG;
	transferData(buff,2);	
	transferData(&hwConfig,sizeof(hwConfig));
}

//====================================
void init_board()
{
	u32 i = 0,j = 0;
	u8 buff[5];
	SIM_RESULT result = SIM_RES_SUCCESS;
	
	
	// Config system clock
	OSCCONbits.SCS0 = 0;		// Use primary clock
	OSCCONbits.SCS1 = 1;
	OSCTUNE = 0x00;				// Use default clock (25Mhz)

	ADCON1 = 0x0F ;
	// init UART
	init_uart1();

	
	
	// Wait factory reset signal
	SIM_STATUS_TRIS = 1;
	STM32_RST_TRIS = 1;
	
	SIM_PWRKEY_TRIS = 0;
	SIM_PWRKEY_IO = 0;
	
	while(i<300)
	{
		if(STM32_RST_IN == 0)
		{
			j = 0;
			while((STM32_RST_IN == 0)&&(j<0xFFFF))
			{
				j++;
				DelayMs(10);
			}
			if(j >= 200)
			{
				// Factory reset
				factoryReset();
				break;
			}
		}
		i++;
		DelayMs(10);
	}
	
	InitAppConfig();
	InitHWConfig();	
	InitUserProfile();
	// Enable global interrupt
	RCONbits.IPEN = 1;				// Enable interrupt priorities
	INTCONbits.GIE = 1;
	INTCONbits.PEIE = 1;	
	
	ClrWdt();	
	
	// Test SIM module if enable GPRS mode
	if((!fConfig)&&(AppConfig.GPRSEnable))
	{
		init_uart2();
		SIM_PowerControl(SIM_POWER_ON);			
		result = testSIM();
		SIM_PowerControl(SIM_POWER_OFF);	
		deinit_uart2();

		if((result == ERROR_SIM_CARD)||(result == ERROR_SIM_MODULE))
		{
			simStatus = SIM_ERROR;

			buff[0] = ETHERNET_TAG;
			buff[1] = ETHERNET_REPORT;
			buff[2] = simStatus;
			transferData(buff,3);	
		}
	}

	SIM_PowerControl(SIM_POWER_OFF);
	
	ClrWdt();
}

//====================================
void transferData(u8 data[],u16 length)
{
	u16 i = 0;

	for(i = 0;i<length;i++)
		uart1_putc(data[i]);
}

//====================================
void factoryReset(void)
{
	fFactoryRST = true;
	// Load default config values
	AppConfig.Flags.bIsDHCPEnabled = TRUE;
	AppConfig.Flags.bInConfigMode = true;
	memcpypgm2ram((void*)&AppConfig.MyMACAddr, (ROM void*)SerializedMACAddress, sizeof(AppConfig.MyMACAddr));

	AppConfig.HostIPAddr.Val = HOST_DEFAULT_IP_ADDR_BYTE1 | HOST_DEFAULT_IP_ADDR_BYTE2<<8ul | HOST_DEFAULT_IP_ADDR_BYTE3<<16ul | HOST_DEFAULT_IP_ADDR_BYTE4<<24ul;
	AppConfig.OperationPort = HOST_OPERATE_PORT;
	AppConfig.EventPort = HOST_EVENT_PORT;
	AppConfig.MyIPAddr.Val = MY_DEFAULT_IP_ADDR_BYTE1 | MY_DEFAULT_IP_ADDR_BYTE2<<8ul | MY_DEFAULT_IP_ADDR_BYTE3<<16ul | MY_DEFAULT_IP_ADDR_BYTE4<<24ul;
	AppConfig.DefaultIPAddr.Val = AppConfig.MyIPAddr.Val;
	AppConfig.MyMask.Val = MY_DEFAULT_MASK_BYTE1 | MY_DEFAULT_MASK_BYTE2<<8ul | MY_DEFAULT_MASK_BYTE3<<16ul | MY_DEFAULT_MASK_BYTE4<<24ul;
	AppConfig.DefaultMask.Val = AppConfig.MyMask.Val;
	AppConfig.MyGateway.Val = MY_DEFAULT_GATE_BYTE1 | MY_DEFAULT_GATE_BYTE2<<8ul | MY_DEFAULT_GATE_BYTE3<<16ul | MY_DEFAULT_GATE_BYTE4<<24ul;
	AppConfig.PrimaryDNSServer.Val = MY_DEFAULT_PRIMARY_DNS_BYTE1 | MY_DEFAULT_PRIMARY_DNS_BYTE2<<8ul  | MY_DEFAULT_PRIMARY_DNS_BYTE3<<16ul  | MY_DEFAULT_PRIMARY_DNS_BYTE4<<24ul;
	AppConfig.SecondaryDNSServer.Val = MY_DEFAULT_SECONDARY_DNS_BYTE1 | MY_DEFAULT_SECONDARY_DNS_BYTE2<<8ul  | MY_DEFAULT_SECONDARY_DNS_BYTE3<<16ul  | MY_DEFAULT_SECONDARY_DNS_BYTE4<<24ul;
	AppConfig.simType = DEFAULT_SIM_TYPE;
	AppConfig.GPRSEnable = false;
	
	// Load the default NetBIOS Host Name
	memcpypgm2ram(AppConfig.NetBIOSName, (ROM void*)MY_DEFAULT_HOST_NAME, 16);
	FormatNetBIOSName(AppConfig.NetBIOSName);

	// Save config to internal flash
	SaveAppConfig();
	flashEraseBlock(USER_ADDRESS);
	fConfig = true;
	fFactoryRST = false;
}

//====================================
static void IP2String(IP_ADDR ip,char* s)
{
	char digits[4];
	char temp[] = ".";
	BYTE i;

	*s = '\0';
	for(i = 0; i < 4u; i++)
	{
		if(i)
			strcat(s,temp);
		uitoa(ip.v[i], digits);
		strcat(s,digits);
	}
}

//====================================
void InitAppConfig(void)
{
	BYTE count = 0;
	u32 addr = 0;
	u16 length = 0;
	u16 i = 0;
		
	AppConfig.Flags.bIsDHCPEnabled = TRUE;
	AppConfig.Flags.bInConfigMode = TRUE;
	AppConfig.GPRSEnable = FALSE;
	memcpypgm2ram((void*)&AppConfig.MyMACAddr, (ROM void*)SerializedMACAddress, sizeof(AppConfig.MyMACAddr));

	AppConfig.HostIPAddr.Val = HOST_DEFAULT_IP_ADDR_BYTE1 | HOST_DEFAULT_IP_ADDR_BYTE2<<8ul | HOST_DEFAULT_IP_ADDR_BYTE3<<16ul | HOST_DEFAULT_IP_ADDR_BYTE4<<24ul;
	AppConfig.OperationPort = HOST_OPERATE_PORT;
	AppConfig.EventPort = HOST_EVENT_PORT;
	AppConfig.MyIPAddr.Val = MY_DEFAULT_IP_ADDR_BYTE1 | MY_DEFAULT_IP_ADDR_BYTE2<<8ul | MY_DEFAULT_IP_ADDR_BYTE3<<16ul | MY_DEFAULT_IP_ADDR_BYTE4<<24ul;
	AppConfig.DefaultIPAddr.Val = AppConfig.MyIPAddr.Val;
	AppConfig.MyMask.Val = MY_DEFAULT_MASK_BYTE1 | MY_DEFAULT_MASK_BYTE2<<8ul | MY_DEFAULT_MASK_BYTE3<<16ul | MY_DEFAULT_MASK_BYTE4<<24ul;
	AppConfig.DefaultMask.Val = AppConfig.MyMask.Val;
	AppConfig.MyGateway.Val = MY_DEFAULT_GATE_BYTE1 | MY_DEFAULT_GATE_BYTE2<<8ul | MY_DEFAULT_GATE_BYTE3<<16ul | MY_DEFAULT_GATE_BYTE4<<24ul;
	AppConfig.PrimaryDNSServer.Val = MY_DEFAULT_PRIMARY_DNS_BYTE1 | MY_DEFAULT_PRIMARY_DNS_BYTE2<<8ul  | MY_DEFAULT_PRIMARY_DNS_BYTE3<<16ul  | MY_DEFAULT_PRIMARY_DNS_BYTE4<<24ul;
	AppConfig.SecondaryDNSServer.Val = MY_DEFAULT_SECONDARY_DNS_BYTE1 | MY_DEFAULT_SECONDARY_DNS_BYTE2<<8ul  | MY_DEFAULT_SECONDARY_DNS_BYTE3<<16ul  | MY_DEFAULT_SECONDARY_DNS_BYTE4<<24ul;
	AppConfig.simType = DEFAULT_SIM_TYPE;
	
	// Load the default NetBIOS Host Name
	memcpypgm2ram(AppConfig.NetBIOSName, (ROM void*)MY_DEFAULT_HOST_NAME, 16);
	FormatNetBIOSName(AppConfig.NetBIOSName);

	// Check if IP config has in internal Flash
	count = sizeof(wnConfig);
	addr = CONFIG_ADDRESS;
	length = CONFIG_INFO_LENGTH;
	for(i = 0;i<length;i++)
	{
		recvBuffer[i] = flashReadByte(addr++);
	}
	if(recvBuffer[2+sizeof(hwConfig)+count] == 0x60)
	{
		m_memcpy((u8*)&AppConfig,(u8*)&recvBuffer[3+sizeof(hwConfig)+count],sizeof(AppConfig));
	}
	else
		SaveAppConfig();
	
	IPServer = AppConfig.HostIPAddr;
	IP2String(AppConfig.HostIPAddr,serverIP);
	uitoa(AppConfig.OperationPort,opPort);	
	uitoa(AppConfig.EventPort,reportPort);	
}

//====================================
void SaveAppConfig(void)
{
	u16 length = 0;
	u32 addr = 0;
	u16 index = 0;
	u16 i = 0;
	
	length = CONFIG_INFO_LENGTH;
	// Read all config info
	for(i = 0;i<512;i++)
		recvBuffer[i] = 0x00;

	addr = CONFIG_ADDRESS;
	for(i = 0;i<length;i++)
	{
		recvBuffer[i] = flashReadByte(addr++);
	}
	m_memcpy((u8*)&recvBuffer[3+sizeof(hwConfig)+sizeof(wnConfig)],(u8*)&AppConfig,sizeof(AppConfig));
	recvBuffer[2+sizeof(hwConfig)+sizeof(wnConfig)] = 0x60;
	index = 0;
	addr = CONFIG_ADDRESS;
	flashEraseBlock(addr);
	for(i=0;i<length;i+= FLASH_WRITE_BLOCK)
	{
		flashWriteBlock(addr,&recvBuffer[index]);
		addr += FLASH_WRITE_BLOCK;
		index += FLASH_WRITE_BLOCK;
	}
	for(i = 0;i<512;i++)
			recvBuffer[i] = 0x00;	

	if((!fFactoryRST)&&(AppConfig.GPRSEnable))
	{
		fConfigSIM = true;
/*	
		init_uart2();
		SIM_PowerControl(SIM_POWER_ON);			
		configSIM(AppConfig.simType);
		SIM_PowerControl(SIM_POWER_OFF);	
		deinit_uart2();
*/		
	}
}

//====================================
void SaveHWConfig(void)
{
	u16 length = 0;
	u32 addr = 0;
	u16 index = 0;
	u16 i = 0;
	
	length = CONFIG_INFO_LENGTH;
	// Read all config info
	for(i = 0;i<512;i++)
			recvBuffer[i] = 0x00;
	
	addr = CONFIG_ADDRESS;
	for(i = 0;i<length;i++)
	{
		recvBuffer[i] = flashReadByte(addr++);
	}
	m_memcpy((u8*)&recvBuffer[1],(u8*)&hwConfig,sizeof(hwConfig));
	recvBuffer[1] = 0x61;
	index = 0;
	addr = CONFIG_ADDRESS;
	flashEraseBlock(addr);
	for(i=0;i<length;i+= FLASH_WRITE_BLOCK)
	{
		flashWriteBlock(addr,&recvBuffer[index]);
		addr += FLASH_WRITE_BLOCK;
		index += FLASH_WRITE_BLOCK;
	}
	for(i = 0;i<512;i++)
			recvBuffer[i] = 0x00;
}


//====================================

void SaveUserProfile(BYTE* cUser, int userLeng, BYTE* cPass, int passLeng)
{
	u16 length = 0;
	u32 addr = 0;
	u16 index = 0;
	u16 i = 0;
	
	length = userLeng + passLeng + 2;
	
	//copy all data to write flash
	for(i = 0;i<512;i++)
			recvBuffer[i] = 0x00;

	recvBuffer[1] = userLeng;
	for(i = 0;i<userLeng;i++)
	{
		recvBuffer[i+2] = cUser[i];
	}

	recvBuffer[userLeng+2] = passLeng;
	for(i = 0;i<passLeng;i++)
	{
		recvBuffer[i+userLeng+3] = cPass[i];
	}
	
	index = 0;
	addr = USER_ADDRESS;
	flashEraseBlock(addr);
	for(i=0;i<length;i+= FLASH_WRITE_BLOCK)
	{
		flashWriteBlock(addr,&recvBuffer[index]);
		addr += FLASH_WRITE_BLOCK;
		index += FLASH_WRITE_BLOCK;
	}

	for(i = 0;i<512;i++)
			recvBuffer[i] = 0x00;	
}

//====================================

void InitUserProfile(void)
{
	u32 addr = 0;
	u8 length = 0;
	u16 i = 0;

	addr = USER_ADDRESS + 1;
	length = flashReadByte(addr++);
	if(length == 0xFF)
		return;

	for(i = 0;i<length;i++)
	{
		username[i] = flashReadByte(addr++);
	}

	length = flashReadByte(addr++);
	for(i = 0;i<length;i++)
	{
		password[i] = flashReadByte(addr++);
	}
	return;
}


//====================================
void InitHWConfig()
{
	u32 addr = 0;
	u16 length = 0;
	u16 i = 0;
	
	hwConfig.ledWidth = DEFAULT_LED_WIDTH;
	hwConfig.ledHeight = DEFAULT_LED_HEIGHT;
	hwConfig.ledType = DEFAULT_LED_TYPE;
	for(i = 0;i<15;i++)
		hwConfig.imei[i] = '1';
	
	addr = CONFIG_ADDRESS;
	length = CONFIG_INFO_LENGTH;
	for(i = 0;i<length;i++)
	{
		recvBuffer[i] = flashReadByte(addr++);
	}
	if(recvBuffer[1] == 0x61)
	{
		m_memcpy((u8*)&hwConfig,(u8*)&recvBuffer[1],sizeof(hwConfig));
	}
	else
		SaveHWConfig();
	//hwConfig.ledWidth = 0;
}

//====================================
bool start_connect(void)
{
	u16 i = 0;
	u16 pageID = 0;
	bool result = false;	
	SIM_RESULT SIM_Result = SIM_RES_SUCCESS;
	
	if(simStatus == SIM_READY)
	{				
		SIM_Result = SIM_SendCmd((rom char *)"AT+CIPSHUT",(rom char *)"OK",(rom char *)"ERROR");
		SIM_Result = GPRS_MultiConnection(true);		
		GPRS_ConfigNetwork();
		// If module sim ready, try connect to server
		i = 0;
		SIM_Result = SIM_RES_ERROR;
		SIM_Result = GPRS_Connect(serverIP,opPort,0);
		if(SIM_Result == SIM_RES_SUCCESS)
			SIM_Result = GPRS_Connect(serverIP,reportPort,1);
		if(SIM_Result == SIM_RES_SUCCESS)
		{
			nwState = NETWORK_GPRS;						
			result = true;
		}
		else
		{
			result = false;
		}
	}
	return result;
}


