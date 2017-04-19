#ifndef _UPDATEFW_H__
#define _UPDATEFW_H__

#define FLAG_ADDR			0x08003800
#define MAIN_APP_ADDR		0x08004000

#define FLAG_UPDATING		0x010A
#define FLAG_UPDATED		0x020D

typedef enum {false = 0,true = 1} bool;

typedef  void (*pFunction)(void);

void updateFW_control(void);
#endif
