/*********************************************************************
 *
 *  Application to Demo HTTP2 Server
 *  Support for HTTP2 module in Microchip TCP/IP Stack
 *	 -Implements the application 
 *	 -Reference: RFC 1002
 *
 *********************************************************************
 * FileName:        CustomHTTPApp.c
 * Dependencies:    TCP/IP stack
 * Processor:       PIC18, PIC24F, PIC24H, dsPIC30F, dsPIC33F, PIC32
 * Compiler:        Microchip C32 v1.05 or higher
 *					Microchip C30 v3.12 or higher
 *					Microchip C18 v3.30 or higher
 *					HI-TECH PICC-18 PRO 9.63PL2 or higher
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * Copyright (C) 2002-2009 Microchip Technology Inc.  All rights
 * reserved.
 *
 * Microchip licenses to you the right to use, modify, copy, and
 * distribute:
 * (i)  the Software when embedded on a Microchip microcontroller or
 *      digital signal controller product ("Device") which is
 *      integrated into Licensee's product; or
 * (ii) ONLY the Software driver source files ENC28J60.c, ENC28J60.h,
 *		ENCX24J600.c and ENCX24J600.h ported to a non-Microchip device
 *		used in conjunction with a Microchip ethernet controller for
 *		the sole purpose of interfacing with the ethernet controller.
 *
 * You should refer to the license agreement accompanying this
 * Software for additional information regarding your rights and
 * obligations.
 *
 * THE SOFTWARE AND DOCUMENTATION ARE PROVIDED "AS IS" WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION, ANY WARRANTY OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * MICROCHIP BE LIABLE FOR ANY INCIDENTAL, SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF
 * PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY OR SERVICES, ANY CLAIMS
 * BY THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE
 * THEREOF), ANY CLAIMS FOR INDEMNITY OR CONTRIBUTION, OR OTHER
 * SIMILAR COSTS, WHETHER ASSERTED ON THE BASIS OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE), BREACH OF WARRANTY, OR OTHERWISE.
 *
 *
 * Author               Date    Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Elliott Wood     	6/18/07	Original
 ********************************************************************/
#define __CUSTOMHTTPAPP_C

#include "TCPIPConfig.h"

#if defined(STACK_USE_HTTP2_SERVER)
#include "TCPIP Stack/TCPIP.h"
#include "Main.h"		// Needed for SaveAppConfig() prototype
#include "typedef.h"
#include "uart.h"
#include "gprs.h"

bool fGetIMEI = false;

extern HW_CONFIG hwConfig;
extern APP_CONFIG AppConfig;
extern WN_CONFIG wnConfig;
extern tREALTIME rts;
extern BYTE username[64];
extern BYTE password[64];
extern WORD dplVersion;
extern WORD mainVersion;
extern IP_ADDR IPServer;
extern bool fIshwConfigRequested;

/****************************************************************************
  Section:
	Function Prototypes and Memory Globalizers
  ***************************************************************************/
static HTTP_IO_RESULT HTTPPostConfigNW(void);
static HTTP_IO_RESULT HTTPPostConfigHW(void);
//static HTTP_IO_RESULT HTTPPostConfigWN(void);
static HTTP_IO_RESULT HTTPPostConfigDT(void);
static HTTP_IO_RESULT HTTPPostChangePW(void);
bool compareByteArray(BYTE arr1[], BYTE arr2[]);

static unsigned int my_atoi(char *p);

unsigned int my_atoi(char *p) 
{
	 unsigned  int k = 0;
	 while (*p) 
	 {
		 k = (k<<3)+(k<<1)+(*p)-'0';
		 p++;
	 }
	 return k;
}

void stringToDate(tREALTIME* rts,char* str)
{
	rts[0].Date = ((str[0]-0x30)<<4|(str[1]-0x30));
	rts[0].Month = ((str[3]-0x30)<<4|(str[4]-0x30));
	rts[0].Year = ((str[8]-0x30)<<4|(str[9]-0x30));
}
void stringToTime(tREALTIME* rts,char* str)
{
	rts[0].Hour= ((str[0]-0x30)<<4|(str[1]-0x30));
	rts[0].Minute = ((str[3]-0x30)<<4|(str[4]-0x30));
	rts[0].Second = ((str[6]-0x30)<<4|(str[7]-0x30));
}

// Sticky status message variable.
// This is used to indicated whether or not the previous POST operation was 
// successful.  The application uses these to store status messages when a 
// POST operation redirects.  This lets the application provide status messages
// after a redirect, when connection instance data has already been lost.
static BOOL lastSuccess = FALSE;
// Stick status message variable.  See lastSuccess for details.
static BOOL lastFailure = FALSE;
unsigned char read_flg = 0;
/****************************************************************************
  Section:
	Authorization Handlers
  ***************************************************************************/
  
/*****************************************************************************
  Function:
	BYTE HTTPNeedsAuth(BYTE* cFile)
	
  Internal:
  	See documentation in the TCP/IP Stack API or HTTP2.h for details.
  ***************************************************************************/
#if defined(HTTP_USE_AUTHENTICATION)
BYTE HTTPNeedsAuth(BYTE* cFile)
{
/*
	// If the filename begins with the folder "protect", then require auth
	if(memcmppgm2ram(cFile, (ROM void*)"protect", 7) == 0)
		return 0x00;		// Authentication will be needed later

	// If the filename begins with the folder "snmp", then require auth
	if(memcmppgm2ram(cFile, (ROM void*)"snmp", 4) == 0)
		return 0x00;		// Authentication will be needed later

	#if defined(HTTP_MPFS_UPLOAD_REQUIRES_AUTH)
	if(memcmppgm2ram(cFile, (ROM void*)"mpfsupload", 10) == 0)
		return 0x00;
	#endif

	if(memcmppgm2ram(cFile, (ROM void*)"index", 5) == 0)
		return 0x00;
*/
	// You can match additional strings here to password protect other files.
	// You could switch this and exclude files from authentication.
	// You could also always return 0x00 to require auth for all files.
	// You can return different values (0x00 to 0x79) to track "realms" for below.

	return 0x00;			// No authentication required
}
#endif

/*****************************************************************************
  Function:
	BYTE HTTPCheckAuth(BYTE* cUser, BYTE* cPass)
	
  Internal:
  	See documentation in the TCP/IP Stack API or HTTP2.h for details.
  ***************************************************************************/
#if defined(HTTP_USE_AUTHENTICATION)
BYTE HTTPCheckAuth(BYTE* cUser, BYTE* cPass)
{
	//if(strcmppgm2ram((char *)cUser, (char *)username)==0//(ROM char *)"admin") == 0 //
	//	&& strcmppgm2ram((char *)cPass, (char *)password)==0)//(ROM char *)"letihn") == 0)//
	//	return 0x80;		// We accept this combination
	
	if(compareByteArray(cUser,username)&&compareByteArray(cPass,password))
		return 0x80;

	// You can add additional user/pass combos here.
	// If you return specific "realm" values above, you can base this 
	//   decision on what specific file or folder is being accessed.
	// You could return different values (0x80 to 0xff) to indicate 
	//   various users or groups, and base future processing decisions
	//   in HTTPExecuteGet/Post or HTTPPrint callbacks on this value.
	
	return 0x00;			// Provided user/pass is invalid
}
#endif

bool compareByteArray(BYTE arr1[], BYTE arr2[])
{
	int i = -1;
	do
	{
		i++;
		if(arr1[i] != arr2[i])
			return FALSE;
		
		if(i > 20)
			return FALSE;
	}while((arr2[i]!='\0')&&(arr1[i]!='\0'));
	
	return TRUE;
}

/****************************************************************************
  Section:
	GET Form Handlers
  ***************************************************************************/
  
/*****************************************************************************
  Function:
	HTTP_IO_RESULT HTTPExecuteGet(void)
	
  Internal:
  	See documentation in the TCP/IP Stack API or HTTP2.h for details.
  ***************************************************************************/
HTTP_IO_RESULT HTTPExecuteGet(void)
{
	BYTE *ptr;
	BYTE filename[20];
	unsigned char i=0;
		
	// Load the file name
	// Make sure BYTE filename[] above is large enough for your longest name
	MPFSGetFilename(curHTTP.file, filename, 20);
	
	return HTTP_IO_DONE;
}


/****************************************************************************
  Section:
	POST Form Handlers
  ***************************************************************************/
  
/*****************************************************************************
  Function:
	HTTP_IO_RESULT HTTPExecutePost(void)
	
  Internal:
  	See documentation in the TCP/IP Stack API or HTTP2.h for details.
  ***************************************************************************/
HTTP_IO_RESULT HTTPExecutePost(void)
{
	// Resolve which function to use and pass along
	BYTE filename[20];
	
	// Load the file name
	// Make sure BYTE filename[] above is large enough for your longest name
	MPFSGetFilename(curHTTP.file, filename, sizeof(filename));

	if(!memcmppgm2ram(filename, "confignw.htm", 12))
		return HTTPPostConfigNW();	
	if(!memcmppgm2ram(filename, "confighw.htm", 12))
			return HTTPPostConfigHW();
	/*
	if(!memcmppgm2ram(filename, "configwn.htm", 12))
			return HTTPPostConfigWN();
	*/
	if(!memcmppgm2ram(filename, "configdt.htm", 12))
			return HTTPPostConfigDT();
	if(!memcmppgm2ram(filename, "configpw.htm", 12))
			return HTTPPostChangePW();

	return HTTP_IO_DONE;
}

/*****************************************************************************
  Function:
	static HTTP_IO_RESULT HTTPPostConfig(void)

  Summary:
	Processes the configuration form on config/index.htm

  Description:
	Accepts configuration parameters from the form, saves them to a
	temporary location in RAM, then eventually saves the data to EEPROM or
	external Flash.
	
	When complete, this function redirects to config/reboot.htm, which will
	display information on reconnecting to the board.

	This function creates a shadow copy of the AppConfig structure in 
	RAM and then overwrites incoming data there as it arrives.  For each 
	name/value pair, the name is first read to curHTTP.data[0:5].  Next, the 
	value is read to newAppConfig.  Once all data has been read, the new
	AppConfig is saved back to EEPROM and the browser is redirected to 
	reboot.htm.  That file includes an AJAX call to reboot.cgi, which 
	performs the actual reboot of the machine.
	
	If an IP address cannot be parsed, too much data is POSTed, or any other 
	parsing error occurs, the browser reloads config.htm and displays an error 
	message at the top.

  Precondition:
	None

  Parameters:
	None

  Return Values:
  	HTTP_IO_DONE - all parameters have been processed
  	HTTP_IO_NEED_DATA - data needed by this function has not yet arrived
  ***************************************************************************/
static HTTP_IO_RESULT HTTPPostConfigNW(void)
{
	APP_CONFIG newAppConfig;
	BYTE *ptr;
	BYTE i;

	if(curHTTP.byteCount > TCPIsGetReady(sktHTTP) + TCPGetRxFIFOFree(sktHTTP))
		goto ConfigFailure;
	
	// Ensure that all data is waiting to be parsed.  If not, keep waiting for 
	// all of it to arrive.
	if(TCPIsGetReady(sktHTTP) < curHTTP.byteCount)
		return HTTP_IO_NEED_DATA;
	
	// Start out assuming that DHCP is disabled.  This is necessary since the 
	// browser doesn't submit this field if it is unchecked (meaning zero).  
	// However, if it is checked, this will be overridden since it will be 
	// submitted.
	memcpy((void*)&newAppConfig,(void*)&AppConfig,sizeof(AppConfig));
	newAppConfig.Flags.bIsDHCPEnabled = TRUE;

	// Read all browser POST data
	while(curHTTP.byteCount)
	{
		// Read a form field name
		if(HTTPReadPostName(curHTTP.data, 6) != HTTP_READ_OK)
			goto ConfigFailure;
			
		// Read a form field value
		if(HTTPReadPostValue(curHTTP.data + 6, sizeof(curHTTP.data)-6-2) != HTTP_READ_OK)
			goto ConfigFailure;
			
		// Parse the value that was read
		if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"ip"))
		{// Read new static IP Address
			if(!StringToIPAddress(curHTTP.data+6, &newAppConfig.MyIPAddr))
				goto ConfigFailure;
			newAppConfig.DefaultIPAddr.Val = newAppConfig.MyIPAddr.Val;
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"gw"))
		{// Read new gateway address
			if(!StringToIPAddress(curHTTP.data+6, &newAppConfig.MyGateway))
				goto ConfigFailure;
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"sub"))
		{// Read new static subnet
			if(!StringToIPAddress(curHTTP.data+6, &newAppConfig.MyMask))
				goto ConfigFailure;

			newAppConfig.DefaultMask.Val = newAppConfig.MyMask.Val;
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"mac"))
		{
			// Read new MAC address
			WORD_VAL w;
			BYTE i;

			ptr = curHTTP.data+6;

			for(i = 0; i < 12u; i++)
			{// Read the MAC address
				
				// Skip non-hex bytes
				while( *ptr != 0x00u && !(*ptr >= '0' && *ptr <= '9') && !(*ptr >= 'A' && *ptr <= 'F') && !(*ptr >= 'a' && *ptr <= 'f') )
					ptr++;

				// MAC string is over, so zeroize the rest
				if(*ptr == 0x00u)
				{
					for(; i < 12u; i++)
						curHTTP.data[i] = '0';
					break;
				}
				
				// Save the MAC byte
				curHTTP.data[i] = *ptr++;
			}
			
			// Read MAC Address, one byte at a time
			for(i = 0; i < 6u; i++)
			{				
				w.v[1] = curHTTP.data[i*2];
				w.v[0] = curHTTP.data[i*2+1];
				newAppConfig.MyMACAddr.v[i] = hexatob(w);
			}
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"host"))
		{// Read new hostIP
			if(!StringToIPAddress(curHTTP.data+6, &newAppConfig.HostIPAddr))
				goto ConfigFailure;			
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"opp"))
		{// Change server port value
			newAppConfig.OperationPort = my_atoi(curHTTP.data+6);
			if(newAppConfig.OperationPort == 0)
				goto ConfigFailure;
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"alp"))
		{// Change server port value
			newAppConfig.EventPort = my_atoi(curHTTP.data+6);
			if(newAppConfig.EventPort == 0)
				goto ConfigFailure;
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"type"))
		{// Read new SIM type
			newAppConfig.simType = my_atoi(curHTTP.data+6);
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"gprs"))
		{
			newAppConfig.GPRSEnable = my_atoi(curHTTP.data+6);
		}
	}
	
	// All parsing complete!  Save new settings and force a reboot		
	{
		//SaveAppConfig(newAppConfig);
		memcpy((void*)&AppConfig,(void*)&newAppConfig,sizeof(AppConfig));
		//fsaveConfig = true;
		SaveAppConfig();
		IPServer = AppConfig.HostIPAddr;
		strcpypgm2ram((char*)curHTTP.data, (ROM char*)"/success.htm");
		curHTTP.httpStatus = HTTP_REDIRECT;
	}	
	return HTTP_IO_DONE;

ConfigFailure:
	lastFailure = TRUE;
	strcpypgm2ram((char*)curHTTP.data, (ROM char*)"/error.htm");
	curHTTP.httpStatus = HTTP_REDIRECT;			
	return HTTP_IO_DONE;
}

static HTTP_IO_RESULT HTTPPostConfigHW(void)
{
	BYTE *ptr;
	BYTE i;

	if(curHTTP.byteCount > TCPIsGetReady(sktHTTP) + TCPGetRxFIFOFree(sktHTTP))
		goto ConfigFailure;
	
	// Ensure that all data is waiting to be parsed.  If not, keep waiting for 
	// all of it to arrive.
	if(TCPIsGetReady(sktHTTP) < curHTTP.byteCount)
		return HTTP_IO_NEED_DATA;
	
	// Use current config in non-volatile memory as defaults
	
	// Read all browser POST data
	while(curHTTP.byteCount)
	{
		// Read a form field name
		if(HTTPReadPostName(curHTTP.data, 6) != HTTP_READ_OK)
			goto ConfigFailure;
			
		// Read a form field value
		if(HTTPReadPostValue(curHTTP.data + 6, sizeof(curHTTP.data)-6-2) != HTTP_READ_OK)
			goto ConfigFailure;
			
		// Parse the value that was read
		if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"imei"))
		{// Read new IMEI number
			memcpy((void*)hwConfig.imei,(void*)(curHTTP.data+6),sizeof(hwConfig.imei));
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"w"))
		{// Read new LED size width
			hwConfig.ledWidth = my_atoi(curHTTP.data+6);
			if(hwConfig.ledWidth == 0)
				goto ConfigFailure;
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"h"))
		{// Read new LED size height
			hwConfig.ledHeight = my_atoi(curHTTP.data+6);
			if(hwConfig.ledHeight == 0)
				goto ConfigFailure;
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"type"))
		{// Read new LED type
			hwConfig.ledType = my_atoi(curHTTP.data+6);
			if(hwConfig.ledType == 0)
				goto ConfigFailure;
		}
	}

	// All parsing complete!  Save new settings and force a reboot			
	for(i = 0;i<15;i++)
	{
		if((hwConfig.imei[i]<'0') || (hwConfig.imei[i]>'9'))
		{
			fGetIMEI = true;
			break;
		}
	}
	if(!fGetIMEI)
	{
		SaveHWConfig();
		SetHWConfig();
	}	
	
	strcpypgm2ram((char*)curHTTP.data, (ROM char*)"/success.htm");
	curHTTP.httpStatus = HTTP_REDIRECT;
	// Set the board to reboot and display reconnecting information	
	return HTTP_IO_DONE;


ConfigFailure:
	lastFailure = TRUE;
	strcpypgm2ram((char*)curHTTP.data, (ROM char*)"/error.htm");
	curHTTP.httpStatus = HTTP_REDIRECT;		
	return HTTP_IO_DONE;
}

static HTTP_IO_RESULT HTTPPostConfigDT(void)
{
	BYTE *ptr;
	BYTE i;
	BYTE data[20];
	
	if(curHTTP.byteCount > TCPIsGetReady(sktHTTP) + TCPGetRxFIFOFree(sktHTTP))
		goto ConfigFailure;
	
	// Ensure that all data is waiting to be parsed.  If not, keep waiting for 
	// all of it to arrive.
	if(TCPIsGetReady(sktHTTP) < curHTTP.byteCount)
		return HTTP_IO_NEED_DATA;
	
	// Use current config in non-volatile memory as defaults
	
	// Read all browser POST data
	while(curHTTP.byteCount)
	{
		// Read a form field name
		if(HTTPReadPostName(curHTTP.data, 6) != HTTP_READ_OK)
			goto ConfigFailure;
			
		// Read a form field value
		if(HTTPReadPostValue(curHTTP.data + 6, sizeof(curHTTP.data)-6-2) != HTTP_READ_OK)
			goto ConfigFailure;
			
		// Parse the value that was read
		if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"date"))
		{
			memset(data,0x00,20);
			memcpy((void*)data,(void*)(curHTTP.data+6),strlen((char*)curHTTP.data+6));
			stringToDate(&rts,data);
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"time"))
		{
			memset(data,0x00,20);
			memcpy((void*)data,(void*)(curHTTP.data+6),strlen((char*)curHTTP.data+6));
			stringToTime(&rts,data);			
		}
	}

	// All parsing complete!  Save new settings 	
	SetDateTime();
	strcpypgm2ram((char*)curHTTP.data, (ROM char*)"/success.htm");
	curHTTP.httpStatus = HTTP_REDIRECT;	
	return HTTP_IO_DONE;

ConfigFailure:
	lastFailure = TRUE;
	strcpypgm2ram((char*)curHTTP.data, (ROM char*)"/error.htm");
	curHTTP.httpStatus = HTTP_REDIRECT;		
	return HTTP_IO_DONE;
}



static HTTP_IO_RESULT HTTPPostChangePW(void)
{
	BYTE *ptr;
	BYTE i;
	BYTE len1 = 0;
	BYTE len2 = 0;
	BYTE opwd[20];
	BYTE npwd[20];
	BYTE cpwd[20];
	
	if(curHTTP.byteCount > TCPIsGetReady(sktHTTP) + TCPGetRxFIFOFree(sktHTTP))
		goto ConfigFailure;
	
	// Ensure that all data is waiting to be parsed.  If not, keep waiting for 
	// all of it to arrive.
	if(TCPIsGetReady(sktHTTP) < curHTTP.byteCount)
		return HTTP_IO_NEED_DATA;
	
	// Use current config in non-volatile memory as defaults
	
	// Read all browser POST data
	while(curHTTP.byteCount)
	{
		// Read a form field name
		if(HTTPReadPostName(curHTTP.data, 6) != HTTP_READ_OK)
			goto ConfigFailure;
			
		// Read a form field value
		if(HTTPReadPostValue(curHTTP.data + 6, sizeof(curHTTP.data)-6-2) != HTTP_READ_OK)
			goto ConfigFailure;
			
		// Parse the value that was read
		if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"opwd"))
		{
			len1 = strlen((char*)curHTTP.data+6);
			if((len1 > 20)|| (len1<=0))
				goto ConfigFailure;
			memset(opwd,0x00,20);
			memcpy((void*)opwd,(void*)(curHTTP.data+6),strlen((char*)curHTTP.data+6));
			for(i= 0; i < len1; i++)
			{
				if(opwd[i]!=password[i])
					goto ConfigFailure;
			}
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"npwd"))
		{
			len1 = strlen((char*)curHTTP.data+6);
			memset(npwd,0x00,20);
			memcpy((void*)npwd,(void*)(curHTTP.data+6),strlen((char*)curHTTP.data+6));;			
		}
		else if(!strcmppgm2ram((char*)curHTTP.data, (ROM char*)"cpwd"))
		{
			len2 = strlen((char*)curHTTP.data+6);
			memset(cpwd,0x00,20);
			memcpy((void*)cpwd,(void*)(curHTTP.data+6),strlen((char*)curHTTP.data+6));
						
		}
	}

	if((len1 > 20) || (len1 <= 0) || (len1 != len2))
		goto ConfigFailure;
	for(i= 0; i < len1; i++)
	{
		if(cpwd[i]!=npwd[i])
			goto ConfigFailure;
	}
	
	for(i= 0; i < 64; i++)
	{
		password[i] = 0x00;
	}
	
	for(i= 0; i < len1; i++)
	{
		password[i] = npwd[i];
	}
	password[len1] = '\0';

	// All parsing complete!  Save new settings 	
	SaveUserProfile(username,6,npwd,len1);
	
	strcpypgm2ram((char*)curHTTP.data, (ROM char*)"/success.htm");
	curHTTP.httpStatus = HTTP_REDIRECT;	
	return HTTP_IO_DONE;

ConfigFailure:
	lastFailure = TRUE;
	strcpypgm2ram((char*)curHTTP.data, (ROM char*)"/error.htm");
	curHTTP.httpStatus = HTTP_REDIRECT;		
	return HTTP_IO_DONE;
}



/****************************************************************************
  Section:
	Dynamic Variable Callback Functions
  ***************************************************************************/

/*****************************************************************************
  Function:
	void HTTPPrint_varname(void)
	
  Internal:
  	See documentation in the TCP/IP Stack API or HTTP2.h for details.
  ***************************************************************************/

/*void HTTPPrint_builddate(void)
{
	curHTTP.callbackPos = 0x01;
	if(TCPIsPutReady(sktHTTP) < strlenpgm((ROM char*)__DATE__" "__TIME__))
		return;
	
	curHTTP.callbackPos = 0x00;
	TCPPutROMString(sktHTTP, (ROM void*)__DATE__" "__TIME__);
}

void HTTPPrint_version(void)
{
	TCPPutROMString(sktHTTP, (ROM void*)version);
}

ROM BYTE HTML_UP_ARROW[] = "up";
ROM BYTE HTML_DOWN_ARROW[] = "dn";

void HTTPPrint_hellomsg(void)
{

	TCPPutROMString(sktHTTP, (ROM BYTE*)"Hello,admin");
	
	return;
}

void HTTPPrint_cookiename(void)
{
	BYTE *ptr;
	
	ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE*)"name");
	
	if(ptr)
		TCPPutString(sktHTTP, ptr);
	else
		TCPPutROMString(sktHTTP, (ROM BYTE*)"not set");
	
	return;
}

void HTTPPrint_uploadedmd5(void)
{
	BYTE i;

	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;
	
	// Make sure there's enough output space
	if(TCPIsPutReady(sktHTTP) < 32u + 37u + 5u)
		return;

	// Check for flag set in HTTPPostMD5
#if defined(STACK_USE_HTTP_MD5_DEMO)
	if(curHTTP.smPost != SM_MD5_POST_COMPLETE)
#endif
	{// No file uploaded, so just return
		TCPPutROMString(sktHTTP, (ROM BYTE*)"<b>Upload a File</b>");
		curHTTP.callbackPos = 0;
		return;
	}
	
	TCPPutROMString(sktHTTP, (ROM BYTE*)"<b>Uploaded File's MD5 was:</b><br />");
	
	// Write a byte of the md5 sum at a time
	for(i = 0; i < 16u; i++)
	{
		TCPPut(sktHTTP, btohexa_high(curHTTP.data[i]));
		TCPPut(sktHTTP, btohexa_low(curHTTP.data[i]));
		if((i & 0x03) == 3u)
			TCPPut(sktHTTP, ' ');
	}
	
	curHTTP.callbackPos = 0x00;
	return;
}*/

extern APP_CONFIG AppConfig;

void HTTPPrintIP(IP_ADDR ip)
{
	BYTE digits[4];
	BYTE i;
	
	for(i = 0; i < 4u; i++)
	{
		if(i)
			TCPPut(sktHTTP, '.');
		uitoa(ip.v[i], digits);
		TCPPutString(sktHTTP, digits);
	}
}

void HTTPPrint_opwd(void)
{
	//TCPPutROMString(sktHTTP, (ROM BYTE*)"");
	return;
}
void HTTPPrint_npwd(void)
{
	//TCPPutROMString(sktHTTP, (ROM BYTE*)"");
	return;
}
void HTTPPrint_cpwd(void)
{
	//TCPPutROMString(sktHTTP, (ROM BYTE*)"");
	return;
}


void HTTPPrint_version_main(void)
{
	BYTE digits[4];
	if((mainVersion<100)||(mainVersion>1000))
		GetVersion();
	uitoa(mainVersion, digits);
	TCPPutString(sktHTTP, digits);
	return;
}

void HTTPPrint_version_network(void)
{
	BYTE digits[4];
	uitoa(FW_VERSION, digits);
	TCPPutString(sktHTTP, digits);
	return;
}

void HTTPPrint_version_dipsplay(void)
{
	BYTE digits[4];
	uitoa(dplVersion, digits);
	TCPPutString(sktHTTP, digits);
	return;
}


void HTTPPrint_config_hostIP(void)
{
	if(AppConfig.HostIPAddr.Val != IPServer.Val)
	{
		AppConfig.HostIPAddr.Val = IPServer.Val;
	}
	HTTPPrintIP(AppConfig.HostIPAddr);
	return;
}

void HTTPPrint_config_dhcpchecked(void)
{
	if(AppConfig.Flags.bIsDHCPEnabled)
		TCPPutROMString(sktHTTP, (ROM BYTE*)"checked");
	return;
}

void HTTPPrint_config_ip(void)
{
	HTTPPrintIP(AppConfig.MyIPAddr);
	return;
}

void HTTPPrint_config_gw(void)
{
	HTTPPrintIP(AppConfig.MyGateway);
	return;
}

void HTTPPrint_config_subnet(void)
{
	HTTPPrintIP(AppConfig.MyMask);
	return;
}


void HTTPPrint_config_imei_disabled(void)
{
	int i = 0, count = 0;
	bool isImeiWrong = false;
	for(i = 0;i<15;i++)
	{
		if((hwConfig.imei[i]<'0') || (hwConfig.imei[i]>'9'))
		{
			isImeiWrong = true;
			break;
		}
		if(hwConfig.imei[i]=='1')
			count++;
	}

	if(count==15)
	{
		isImeiWrong = true;
	}

	if(isImeiWrong)
		TCPPutROMString(sktHTTP, (ROM BYTE*)" ");
	else 
		TCPPutROMString(sktHTTP, (ROM BYTE*)"disabled");
	
	return;
}

void HTTPPrint_config_imei(void)
{
	int i = 0, count = 0;
	bool isImeiWrong = false;
	BYTE data[DEFAULT_IMEI_SIZE + 1];
	//GetHWConfig();
	if(!fIshwConfigRequested)
	{
		GetHWConfig();
		fIshwConfigRequested = true;
	}

	
	
	for(i = 0;i<15;i++)
	{
		if((hwConfig.imei[i]<'0') || (hwConfig.imei[i]>'9'))
		{
			isImeiWrong = true;
			break;
		}
		if(hwConfig.imei[i]=='1')
			count++;
	}

	if(count==15)
	{
		isImeiWrong = true;
	}

	if(isImeiWrong)
	{
		//memcpy((void*)data,"111111111111111",DEFAULT_IMEI_SIZE);
		memset((void*)data,'x',DEFAULT_IMEI_SIZE);
	}
	else
	{
		memcpy((void*)data,(void*)hwConfig.imei,DEFAULT_IMEI_SIZE);
	}

	data[DEFAULT_IMEI_SIZE] = 0;
	TCPPutString(sktHTTP,data);
}

void HTTPPrint_config_width(void)
{
	BYTE digits[4];
	BYTE i;
	
	uitoa(hwConfig.ledWidth, digits);
	TCPPutString(sktHTTP, digits);
}

void HTTPPrint_config_height(void)
{
	BYTE digits[4];
	BYTE i;
	
	uitoa(hwConfig.ledHeight, digits);
	TCPPutString(sktHTTP, digits);
}

void HTTPPrint_config_type(void)
{
	BYTE digits[4];
	
	uitoa(hwConfig.ledType, digits);
	TCPPutString(sktHTTP, digits);
}

void HTTPPrint_config_opport(void)
{
	BYTE digits[5];
	
	uitoa(AppConfig.OperationPort, digits);
	TCPPutString(sktHTTP, digits);
}

void HTTPPrint_config_alport(void)
{
	BYTE digits[5];
	
	uitoa(AppConfig.EventPort, digits);
	TCPPutString(sktHTTP, digits);
}

void HTTPPrint_config_mac(void)
{
	BYTE i;
	
	if(TCPIsPutReady(sktHTTP) < 18u)
	{//need 17 bytes to write a MAC
		curHTTP.callbackPos = 0x01;
		return;
	}	
	
	// Write each byte
	for(i = 0; i < 6u; i++)
	{
		if(i)
			TCPPut(sktHTTP, ':');
		TCPPut(sktHTTP, btohexa_high(AppConfig.MyMACAddr.v[i]));
		TCPPut(sktHTTP, btohexa_low(AppConfig.MyMACAddr.v[i]));
	}
	
	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;
}

void HTTPPrint_reboot(void)
{
/*
	if(fsaveConfig)
		SaveAppConfig();
*/
}

void HTTPPrint_scan_1()
{
	if(hwConfig.ledType == 1)
	{
		TCPPutROMString(sktHTTP, (ROM BYTE*)"selected");
	}
	else
	{
		TCPPutROMString(sktHTTP, (ROM BYTE*)" ");
	}
}

void HTTPPrint_scan_2()
{
	if(hwConfig.ledType == 2)
	{
		TCPPutROMString(sktHTTP, (ROM BYTE*)"selected");
	}
	else
		TCPPutROMString(sktHTTP, (ROM BYTE*)" ");
}

void HTTPPrint_scan_3()
{
	if(hwConfig.ledType == 3)
	{
		TCPPutROMString(sktHTTP, (ROM BYTE*)"selected");
	}
	else
		TCPPutROMString(sktHTTP, (ROM BYTE*)" ");
}

void HTTPPrint_scan_4()
{
	if(hwConfig.ledType == 4)
	{
		TCPPutROMString(sktHTTP, (ROM BYTE*)"selected");
	}
	else
		TCPPutROMString(sktHTTP, (ROM BYTE*)" ");
}

void HTTPPrint_sim_1(void)
{
	if(AppConfig.simType == SIM_VIETTEL)
	{
		TCPPutROMString(sktHTTP, (ROM BYTE*)"selected");
	}
	else
	{
		TCPPutROMString(sktHTTP, (ROM BYTE*)" ");
	}
}

void HTTPPrint_sim_2(void)
{
	if(AppConfig.simType == SIM_VINAPHONE)
	{
		TCPPutROMString(sktHTTP, (ROM BYTE*)"selected");
	}
	else
	{
		TCPPutROMString(sktHTTP, (ROM BYTE*)" ");
	}
}

void HTTPPrint_sim_3(void)
{
	if(AppConfig.simType == SIM_MOBIFONE)
	{
		TCPPutROMString(sktHTTP, (ROM BYTE*)"selected");
	}
	else
	{
		TCPPutROMString(sktHTTP, (ROM BYTE*)" ");
	}
}

void HTTPPrint_date(void)
{
	BYTE digits[4] = "";
	BYTE i = 0;
	char temp = 0;
	GetDateTime();
	temp = (rts.Date>>4)*10 + (rts.Date&0x0F);
	TCPPut(sktHTTP, (temp/10 + 0x30));
	TCPPut(sktHTTP, (temp%10 + 0x30));
	TCPPut(sktHTTP, '/');
	temp = (rts.Month>>4)*10 + (rts.Month&0x0F);
	TCPPut(sktHTTP, (temp/10 + 0x30));
	TCPPut(sktHTTP, (temp%10 + 0x30));
	TCPPut(sktHTTP, '/');
	temp = (rts.Year>>4)*10 + (rts.Year&0x0F);
	uitoa(20, digits);
	TCPPutString(sktHTTP, digits);
	TCPPut(sktHTTP, (temp/10 + 0x30));
	TCPPut(sktHTTP, (temp%10 + 0x30));
}
void HTTPPrint_time(void)
{
	BYTE digits[4] = "";
	BYTE i = 0;
	char temp = 0;

	temp = (rts.Hour>>4)*10 + (rts.Hour&0x0F);
	TCPPut(sktHTTP, (temp/10 + 0x30));
	TCPPut(sktHTTP, (temp%10 + 0x30));
	TCPPut(sktHTTP, ':');
	temp = (rts.Minute>>4)*10 + (rts.Minute&0x0F);
	TCPPut(sktHTTP, (temp/10 + 0x30));
	TCPPut(sktHTTP, (temp%10 + 0x30));
	TCPPut(sktHTTP, ':');
	temp = (rts.Second>>4)*10 + (rts.Second&0x0F);
	TCPPut(sktHTTP, (temp/10 + 0x30));
	TCPPut(sktHTTP, (temp%10 + 0x30));
}

void HTTPPrint_gprs_1(void)
{
	if(AppConfig.GPRSEnable)
	{
		TCPPutROMString(sktHTTP, (ROM BYTE*)"selected");
	}
	else
	{
		TCPPutROMString(sktHTTP, (ROM BYTE*)" ");
	}
}

void HTTPPrint_gprs_2(void)
{
	if(!AppConfig.GPRSEnable)
	{
		TCPPutROMString(sktHTTP, (ROM BYTE*)"selected");
	}
	else
	{
		TCPPutROMString(sktHTTP, (ROM BYTE*)" ");
	}
}

#endif
