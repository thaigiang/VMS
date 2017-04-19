#ifndef __GPRS_H_
#define __GPRS_H_


#define SIM_STATUS_IO		PORTCbits.RC2
#define SIM_STATUS_TRIS		TRISCbits.TRISC2
#define SIM_PWRKEY_IO		LATDbits.LATD0
#define SIM_PWRKEY_TRIS		TRISDbits.TRISD0

typedef enum
{
	SIM_RES_SUCCESS =0,
	SIM_RES_ERROR,
	ERROR_SIM_CARD,
	ERROR_SIM_MODULE
} SIM_RESULT;

typedef enum
{
	SIM_POWER_ON = 0,
	SIM_POWER_OFF
} SIM_POWER_STATE;

typedef enum
{
	SIM_READY = 0,
	SIM_NOT_READY,
	SIM_OFF,
	SIM_ERROR
} SIM_STATUS;
//===================================================
void configSIM(SIM_TYPE simType);
SIM_RESULT GPRS_MultiConnection(bool enable);
void GPRS_ConfigNetwork(void);
SIM_RESULT GPRS_Connect(char* ipaddress,char* port,u8 num);
SIM_RESULT GPRS_SendData(u8 conn_num,u8* buff,u16 length);
SIM_RESULT SIM_GetIMEI(u8 *imei);
SIM_STATUS SIM_PowerControl(SIM_POWER_STATE state);
SIM_RESULT SIM_SendCmd(rom char* cmd,rom char* exp_success,rom char* exp_error);
SIM_RESULT testSIM(void);
bool str_cmpx(unsigned char* str1,u16 length1,rom char* str2,u16 length2);
//====================================================

#endif
