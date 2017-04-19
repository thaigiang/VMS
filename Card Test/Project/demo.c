/***************************************************************************
 * STM32 VGA demo
 * Copyright (C) 2012 Artekit Italy
 * http://www.artekit.eu
 * Written by Ruben H. Meleca
 
### demo.c
 
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

***************************************************************************/
#include <string.h>
#include <stdlib.h>
#include "stm32f10x.h"
#include "sys.h"
#include "video.h"
#include "gdi.h"
#include "stm32f10x_conf.h"

#define X_START			3
#define Y_START			2

extern volatile u32		sysTicks;

//u8 ledData[6912];

void demoInit(void) 
{
	u16 i,j;
	u16 value = 0;
	u16 x = 0;

//	memset(ledData,0xAA,6912);
/*	
	gdiRectangle(0,0,(VID_PIXELS_X - 1),VID_VSIZE - 1,0,GREEN_COLOR);
	
	gdiDrawTextEx(2,0, "STM32F103VET6", GDI_ROP_COPY,RED_COLOR);
	gdiDrawTextEx(22, 55, "FTS CARD TEST FOR LED MATRIX", GDI_ROP_COPY,RED_COLOR);
	gdiDrawTextEx(62, 75, "VMS PROJECT", GDI_ROP_COPY,RED_COLOR);

	gdiDrawTextEx(0,0, "STM32F103VET6", GDI_ROP_COPY,GREEN_COLOR);
	gdiDrawTextEx(20, 55, "FTS CARD TEST FOR LED MATRIX", GDI_ROP_COPY,GREEN_COLOR);
	gdiDrawTextEx(60, 75, "VMS PROJECT", GDI_ROP_COPY,GREEN_COLOR);
*/
/*
	vidClearScreen();
	//gdiRectangle(2,26,195,171,0,GREEN_COLOR);
	for(i = 0;i<6912;i+=4)
	{
		ledData[i] = 0xAA;
		ledData[i+1] = 0xAA;
		ledData[i+2] = 0x55;
		ledData[i+3] = 0x55;
	}
	vidClearScreen();
	for(i = 0;i<144;i++)
	{
		for(j=0;j<192;j++)
		{
			x = j*2;
			value = getMatrixPixel(ledData,192,j,i);
			if(value & 0x01)
			{
				gdiPoint(8 + x,Y_START + i,0,RED_COLOR);
				gdiPoint(8 + x+1,Y_START + i,0,RED_COLOR);
			}
			if(value & 0x02)
			{
				gdiPoint(10 + x,Y_START + i,0,GREEN_COLOR);
				gdiPoint(10 + x+1,Y_START + i,0,GREEN_COLOR);
			}
		}
	}
*/	
	while(1) {};
}

