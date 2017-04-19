#include <String.h>
#include <p18cxxx.h>
#include "TCPIP Stack/TCPIP.h"
#include "typedef.h"
#include "gprs.h"
#include "main.h"
#include "uart.h"

extern BYTE recvBuffer[512];
extern u8 simResCnt;
extern bool fReceive;

bool fSendCmd = false;

//====================================
//static void SIM_Send(u8* buff);
//static u8 SIM_Receive(void);
static void convert(unsigned int num, int base,char *buff);

//====================================
void configSIM(SIM_TYPE simType)
{
	SIM_SendCmd((rom char*)"AT",(rom char*)"OK",(rom char*)"ERROR");
	SIM_SendCmd((rom char*)"AT",(rom char*)"OK",(rom char*)"ERROR");
	SIM_SendCmd((rom char*)"AT",(rom char*)"OK",(rom char*)"ERROR");
	SIM_SendCmd((rom char*)"AT",(rom char*)"OK",(rom char*)"ERROR");
	SIM_SendCmd((rom char*)"AT",(rom char*)"OK",(rom char*)"ERROR");
	SIM_SendCmd((rom char*)"ATE0",(rom char*)"OK",(rom char*)"ERROR");
	SIM_SendCmd((rom char*)"AT&W",(rom char*)"OK",(rom char*)"ERROR");
	SIM_SendCmd((rom char*)"AT+CNMI=2,0,0,0,0",(rom char*)"OK",(rom char*)"ERROR");	
	SIM_SendCmd((rom char*)"AT+CSAS",(rom char*)"OK",(rom char*)"ERROR");
	switch(simType)
	{
		case SIM_VIETTEL:
			SIM_SendCmd((rom char*)"AT+CIPCSGP=1,\"v-internet\",,",(rom char*)"OK",(rom char*)"ERROR");
			break;
		case SIM_VINAPHONE:
			SIM_SendCmd((rom char*)"AT+CIPCSGP=1,\"m3-world\",\"mms\",\"mms\"",(rom char*)"OK",(rom char*)"ERROR");
			break;
		case SIM_MOBIFONE:
			SIM_SendCmd((rom char*)"AT+CIPCSGP=1,\"m-wap\",\"mms\",\"mms\"",(rom char*)"OK",(rom char*)"ERROR");
			break;
		default:
			break;
	}
	SIM_SendCmd((rom char*)"AT+CIPHEAD=0",(rom char*)"OK",(rom char*)"ERROR");
	SIM_SendCmd((rom char*)"AT+CIPSCONT",(rom char*)"OK",(rom char*)"ERROR");	
	SIM_SendCmd((rom char*)"AT+CMEE=0",(rom char*)"OK",(rom char*)"ERROR");	
}

//===================================
SIM_RESULT GPRS_MultiConnection(bool enable)
{
	SIM_RESULT result = SIM_RES_SUCCESS;

	if(enable)
	{
		// Send command to SIM	
		result = SIM_SendCmd((rom char*)"AT+CIPMUX=1",(rom char*)"OK",(rom char*)"ERROR");
	}
	else
		result = SIM_SendCmd((rom char*)"AT+CIPMUX=0",(rom char*)"OK",(rom char*)"ERROR");
	
	return result;
}

//===================================
void GPRS_ConfigNetwork(void)
{
	u16 i = 0;

	DelayMs(1000);
	SIM_SendCmd((rom char*)"AT+CSTT",(rom char*)"OK",(rom char*)"ERROR");
	DelayMs(1000);
	SIM_SendCmd((rom char*)"AT+CIICR",(rom char*)"OK",(rom char*)"ERROR");
	DelayMs(2000);
	fSendCmd = true;	
	uart2_putROMString((rom char*)"AT+CIFSR\r\n");	
	DelayMs(2000);
	for(i = 0;i<simResCnt;i++)
		recvBuffer[i] = 0x00;
	simResCnt = 0;
	fReceive = false;
	fSendCmd = false;
}

//===================================
SIM_RESULT GPRS_Connect(char* ipaddress,char* port,u8 num)
{
	SIM_RESULT result = SIM_RES_SUCCESS;
	u16 i = 0;

	fSendCmd = true;
	
	// Send command to SIM	
	uart2_putROMString((rom char*)"AT+CIPSTART=");
	num += 0x30;
	uart2_putc(num);
	uart2_putROMString((rom char*)",\"TCP\",\"");
	i = 0;
	while(ipaddress[i] != '\0')
		uart2_putc(ipaddress[i++]);
	
	uart2_putROMString((rom char*)"\",\"");
	i = 0;
	while(port[i] != '\0')
		uart2_putc(port[i++]);
	
	uart2_putROMString((rom char*)"\"\r\n");

	// Wait response
	i = 0;
	while(i<500)
	{
		if(fReceive)
		{
			if(str_cmpx(recvBuffer,simResCnt,(rom char*)"CONNECT FAIL",12))
			{
				result = SIM_RES_ERROR;
				break;
			}		

			if(str_cmpx(recvBuffer,simResCnt,(rom char*)"ERROR",5))
			{
				result = SIM_RES_ERROR;
				break;
			}	
				
			if(str_cmpx(recvBuffer,simResCnt,(rom char*)"CONNECT OK",10))
			{
				result = SIM_RES_SUCCESS;
				break;
			}

			if(str_cmpx(recvBuffer,simResCnt,(rom char*)"ALREADY CONNECT",15))
			{
				result = SIM_RES_ERROR;
				break;
			}
		}
		DelayMs(10);
		i++;
	}

	if(i>=500)
	{
		result = SIM_RES_ERROR;
	}

	for(i = 0;i<simResCnt;i++)
		recvBuffer[i] = 0x00;	
	simResCnt = 0;
	fReceive = false;	
	fSendCmd = false;
	
	return result;
}
//===================================
SIM_RESULT GPRS_SendData(u8 conn_num,u8* buff,u16 length)
{
	SIM_RESULT result = SIM_RES_SUCCESS;
	char s[10];
	u16 i = 0,j = 0;
	u8 c = 0;

	fSendCmd = true;
	
	for(i = 0;i<10;i++)
		s[i] = 0;
	
	convert(length,10,&s[0]);
	
	// Send command
	uart2_putROMString((rom char*)"AT+CIPSEND=");
	uart2_putc(conn_num+0x30);
	uart2_putc(',');

	i = 0;
	while(s[i] != 0)
	{
		uart2_putc(s[i]);
		i++;
	}

	uart2_putROMString((rom char*)"\r");

	i = 0;
	while(i<400)
	{
		if(str_cmpx(recvBuffer,simResCnt,(rom char*)">",1))
			break;
		DelayMs(5);
		i++;
	}

	if(i < 400)
	{
		for(j = 0;j<length;j++)
		{
			uart2_putc(buff[j]);
		}
		uart2_putc(26);
	}

	fSendCmd = false;
	fReceive = false;
	for(i = 0;i<simResCnt;i++)
		recvBuffer[i] = 0x00;
	simResCnt = 0;	

	return result;
}

//===================================
SIM_RESULT SIM_GetIMEI(u8 *imei)
{	
	u16 i = 0;
	u8 index = 0;
	
	fSendCmd = true;		
	// Send command to SIM	
	uart2_putROMString((rom char*)"AT+GSN\r\n");

	while(!fReceive)
		DelayMs(50);
	while((recvBuffer[index]<0x30)||(recvBuffer[index]>0x39))
		index ++;
	
	memcpy(imei,(const void *)&recvBuffer[index],15);
	
	for(i = 0;i<simResCnt;i++)
		recvBuffer[i] = 0x00;
	simResCnt = 0;
	fReceive = false;
	fSendCmd = false;
}

//===================================
SIM_STATUS SIM_PowerControl(SIM_POWER_STATE state)
{
	u32 i = 0,j = 0;
	bool fNeedPulse =false;
	SIM_STATUS result = SIM_NOT_READY;
	u32 timeOut = 0;
	
	switch(state)
	{
		case SIM_POWER_ON:
			// Check SIM status
			if(SIM_STATUS_IO == 0)
			{
				fNeedPulse = true;
			}
			else
				result = SIM_READY;
			
			break;
		case SIM_POWER_OFF:
			// Check SIM status
			if(SIM_STATUS_IO == 1)
			{
				fNeedPulse = true;
			}
			else
				result = SIM_OFF;

			break;
	}
	
	if(fNeedPulse == true)
	{
		fSendCmd = true;
		
		// Power up/down GSM/GPRS module
		SIM_PWRKEY_IO = 1;

		DelayMs(2000);
		
		SIM_PWRKEY_IO = 0;
		// Wait response from module SIM
		if(state == SIM_POWER_OFF)
		{
			// Wait response from module SIM
			i = 0;
			while((i < 500) &&(SIM_STATUS_IO == 1))
			{				
				DelayMs(10);
				i++;
			}
			if(i < 500)
				result = SIM_OFF;
			else 
				result = SIM_NOT_READY;
		}
		else if(state == SIM_POWER_ON)
		{
			// Wait response from module SIM
			i = 0;
			while((i < 500) &&(SIM_STATUS_IO == 0))
			{				
				DelayMs(10);
				i++;
			}
			if(i < 500)
			{
				// Wait SIM ready
				DelayMs(3000);
				result = SIM_READY;
			}
			else 
				result = SIM_NOT_READY;
		}
		
		for(i = 0;i<simResCnt;i++)
			recvBuffer[i] = 0x00;
		
		simResCnt = 0;
		fReceive = false;
		fSendCmd = false;
	}
	
	return result;
}

//===================================
SIM_RESULT testSIM(void)
{
	u16 i = 0;
	u8 index = 0;
	SIM_RESULT result = SIM_RES_SUCCESS;	

	// Test AT command
	result = SIM_SendCmd((rom char*)"AT",(rom char*)"OK",(rom char*)"ERROR");
	result = SIM_SendCmd((rom char*)"AT",(rom char*)"OK",(rom char*)"ERROR");
	result = SIM_SendCmd((rom char*)"AT",(rom char*)"OK",(rom char*)"ERROR");
	if(result == SIM_RES_SUCCESS)
	{
		fSendCmd = true;	
		// Check SIM card status
		uart2_putROMString((rom char*)"AT+CSMINS?\r\n");

		// Wait response
		i = 0;
		while(i<500)
		{
			if(fReceive)
			{
				if(str_cmpx(recvBuffer,simResCnt,(rom char*)"+CSMINS: 0,0",12))
				{
					result = ERROR_SIM_CARD;
					break;
				}		

				if(str_cmpx(recvBuffer,simResCnt,(rom char*)"+CSMINS: 1,0",12))
				{
					result = ERROR_SIM_CARD;
					break;
				}			

				if(str_cmpx(recvBuffer,simResCnt,(rom char*)"+CSMINS: 0,1",12))
				{
					result = SIM_RES_SUCCESS;
					break;
				}	

				if(str_cmpx(recvBuffer,simResCnt,(rom char*)"+CSMINS: 1,1",12))
				{
					result = SIM_RES_SUCCESS;
					break;
				}	
			}
			DelayMs(10);
			i++;
		}

		if(i>=500)
			result = SIM_RES_ERROR;
	
		for(i = 0;i<simResCnt;i++)
			recvBuffer[i] = 0x00;
		simResCnt = 0;
		fReceive = false;
		fSendCmd = false;
	}
	else
		result = ERROR_SIM_MODULE;
	
	return result;
}

/*
//===================================
void SIM_Send(u8* buff)
{
	while(*buff != 0)
	{		   
		uart2_putc(*buff);
		buff++;
	}
}

//===================================
u8 SIM_Receive(void)
{
	u8 c;

	c = uart2_getc();
	return c;	
}*/

SIM_RESULT SIM_SendCmd(rom char* cmd,rom char* exp_success,rom char* exp_error)
{
	u16 i = 0;
	u8 index = 0;
	SIM_RESULT result = SIM_RES_SUCCESS;
	
	fSendCmd = true;		
	// Send command to SIM	
	uart2_putROMString(cmd);
	uart2_putROMString((rom char*)"\r\n");
	// Wait response
	i = 0;
	while(i<200)
	{
		if(fReceive)
		{
			if(str_cmpx(recvBuffer,simResCnt,exp_error,strlen((const char *)exp_error)))
			{
				result = SIM_RES_ERROR;
				break;
			}	
				
			if(str_cmpx(recvBuffer,simResCnt,exp_success,strlen((const char *)exp_success)))
			{
				result = SIM_RES_SUCCESS;
				break;
			}
		}
		DelayMs(10);
		i++;
	}

	if(i>=200)
	{
		result = SIM_RES_ERROR;
	}
	
	for(i = 0;i<simResCnt;i++)
		recvBuffer[i] = 0x00;
	simResCnt = 0;
	fReceive = false;
	fSendCmd = false;

	return result;
}

//=========================================================
bool str_cmpx(unsigned char* str1,u16 length1,rom char* str2,u16 length2)
{
	unsigned int i,j = 0;
	unsigned char index = 0;
	bool result = false;
	unsigned char c1,c2;
	
	for(i=0;i<length1;i++)
	{
		c1 = str1[i];
		c2 = str2[0];
		if(c1 == c2)
		{
			index  = i;
			for(j=0;j<length2;j++)
			{
				if(str1[index+j]!=str2[j])
				{
					result = false;
					break;
				}
			}
			if(j == length2)
			{
				result = true;
				break;
			}
		}
	}

	return result;
}

void convert(unsigned int num, int base,char *buff)
{
	static char s[33];
	char *ptr;
	u8 count = 0;
	u8 i = 0;
	
	ptr=&s[sizeof(s)-1];
	*ptr='\0';
	do
	{
		*--ptr="0123456789ABCDEF"[num%base];
		num/=base;
		count ++;
	}while(num!=0);

	for(i = 0;i<count;i++)
		buff[i] = ptr[i];
}

