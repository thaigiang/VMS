/*********************************************************************
 *
 *  Berekely TCP lient demo application.
 *  This application uses the BSD socket APIs and start a client
 *
 *********************************************************************
 * FileName:        BerkeleyTCPClientDemo.c
 * Company:         Microchip Technology, Inc.
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
 * Author               Date    	Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Aseem Swalah         4/17/08  	Original
 ********************************************************************/

#include "TCPIPConfig.h" 
#include "uart.h"
#include "typedef.h"

#if defined(STACK_USE_BERKELEY_API)

#include "TCPIP Stack/TCPIP.h"
#include "transfer.h"


#define USE_PORT_EVENT		1

BSD_STATE  BSDClientState = BSD_START;

extern NETWORK_STATUS nwState;

SOCKET bsdOperateSocket;
#ifdef USE_PORT_EVENT
SOCKET bsdEventSocket;
#endif
BYTE recvBuffer[512];
extern bool fSendState;
extern APP_CONFIG AppConfig;
/*********************************************************************
 * Function:        void BerkeleyTCPClientDemo(void)
 *
 * PreCondition:    Stack is initialized()
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
void BerkeleyTCPClient(void)
{
    static struct sockaddr_in addr;
    int i;
    int addrlen;
	int err_code = 0;	
	static int tCount1 = 0,tCount2 = 0;       

    switch(BSDClientState)
    {	    
        case BSD_START:
            // Create a socket for this client to connect with 
            #ifdef USE_PORT_EVENT
         	if((bsdEventSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET )
                return;         
			#endif
            if((bsdOperateSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET )
                return;			
            BSDClientState = BSD_CONNECT;
            break;

        case BSD_CONNECT:            
            addr.sin_port = AppConfig.OperationPort;
            addrlen = sizeof(struct sockaddr);
			addr.sin_addr.S_un.S_un_b.s_b1 = (BYTE)AppConfig.HostIPAddr.byte.LB; 
			addr.sin_addr.S_un.S_un_b.s_b2 = (BYTE)AppConfig.HostIPAddr.byte.HB; 
			addr.sin_addr.S_un.S_un_b.s_b3 = (BYTE)AppConfig.HostIPAddr.byte.UB; 
			addr.sin_addr.S_un.S_un_b.s_b4 = (BYTE)AppConfig.HostIPAddr.byte.MB; 
			err_code = connect( bsdOperateSocket, (struct sockaddr*)&addr, addrlen);
            if(err_code < 0)
            {
            	tCount1 ++;
				if(tCount1 >= 50)
				{
					BSDClientState = BSD_CLOSE;
					tCount1 = 0;
				}
				return;
            }
			#ifdef USE_PORT_EVENT
			addr.sin_port = AppConfig.EventPort;
			err_code = connect( bsdEventSocket, (struct sockaddr*)&addr, addrlen);
			if(err_code < 0)
            {
            	tCount2 ++;
				if(tCount2 >= 50)
				{
					BSDClientState = BSD_CLOSE;
					tCount2 = 0;
				}
				return;
            }
			#endif
            BSDClientState = BSD_OPERATION;
			nwState = NETWORK_ETHERNET;
			fSendState = false;
        
        case BSD_OPERATION:
            // Obtian and print the server reply
            while(1)
            {
				i = recv(bsdOperateSocket, recvBuffer, sizeof(recvBuffer), 0); //get the data from the recv queue

                if(i == 0)
					break;
                
                if(i< 0) //error condition
                {
                    BSDClientState = BSD_CLOSE;
					if(nwState == NETWORK_ETHERNET)
						nwState = NETWORK_LOST;
					fSendState = false;
                    break;
                }

				if(i>0)
				{			
					u32 j = 0;
					for(j=0;j<i;j++)
						uart1_putc(recvBuffer[j]);
				}
            }
            break;
         
        case BSD_CLOSE:
            closesocket(bsdOperateSocket);
			#ifdef USE_PORT_EVENT
			closesocket(bsdEventSocket);
			#endif
            BSDClientState = BSD_DONE;
            // No break needed
            
        case BSD_DONE:
			BSDClientState = BSD_START;
            break;
         
        default:
            return;
    }
}

#endif //#if defined(STACK_USE_BERKELEY_API)

