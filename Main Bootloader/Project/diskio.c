/*-----------------------------------------------------------------------*/
/* MMC/SDSC/SDHC (in SPI mode) control module for STM32 Version 1.1.6    */
/* (C) Martin Thomas, 2010 - based on the AVR MMC module (C)ChaN, 2007   */
/*-----------------------------------------------------------------------*/
/************************************************************/
/*Giao tiep phan cung voi sd card bang mode SPI */
/*************************************************************/
/*-------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_gpio.h"
#include "diskio.h"
#include "typedef.h"
#include "HardwareConfig.h"

/*----------------------------------------------------*/
/* cau hinh phan cung cho board */
/*-----------------------------------------------------*/
#define SDCARD_SPI						SPI1
#define SPI_BaudRatePrescaler_SPI_SD	0x04

/* Definitions for MMC/SDC command */
#define CMD0	(0x40+0)	/* GO_IDLE_STATE */
#define CMD1	(0x40+1)	/* SEND_OP_COND (MMC) */
#define ACMD41	(0xC0+41)	/* SEND_OP_COND (SDC) */
#define CMD8 	(0x40+8)	/* SEND_IF_COND */
#define CMD9 	(0x40+9)	/* SEND_CSD */
#define CMD10	(0x40+10)	/* SEND_CID */
#define CMD12	(0x40+12)	/* STOP_TRANSMISSION */
#define CMD13	(0x40+13)		/* SEND_STATUS */
#define ACMD13	(0xC0+13)	/* SD_STATUS (SDC) */
#define CMD16	(0x40+16)	/* SET_BLOCKLEN */
#define CMD17	(0x40+17)	/* READ_SINGLE_BLOCK */
#define CMD18	(0x40+18)	/* READ_MULTIPLE_BLOCK */
#define CMD23	(0x40+23)	/* SET_BLOCK_COUNT (MMC) */
#define ACMD23	(0xC0+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(0x40+24)	/* WRITE_BLOCK */
#define CMD25	(0x40+25)	/* WRITE_MULTIPLE_BLOCK */
#define CMD55	(0x40+55)	/* APP_CMD */
#define CMD58	(0x40+58)	/* READ_OCR */

/* cau hinh chan CS cho sd card */
#define SELECT()        (SDC_CS_PORT->BRR = SDC_CS_PIN)    	/* MMC CS = L */
#define DESELECT()      (SDC_CS_PORT->BSRR = SDC_CS_PIN)      /* MMC CS = H */


static
DSTATUS Stat = STA_NOINIT;	/* Disk status */

static
BYTE CardType;			/* b0:MMC, b1:SDv1, b2:SDv2, b3:Block addressing */

/*-----------------------------------------------------------------------*/
/* Transmit a byte to MMC via SPI  (Platform dependent)                  */
/*-----------------------------------------------------------------------*/
#define xmit_spi(dat)  spi_rw(dat)

/* Alternative macro to receive data fast */
/* dest is address of the memory where held response*/
#define rcvr_spi_m(dst)  *(dst)=spi_rw(0xff)


/*--------------------------------------------------------------------------

   Module Private Functions and Variables

---------------------------------------------------------------------------*/

enum speed_setting { INTERFACE_SLOW, INTERFACE_FAST };

static void interface_speed( enum speed_setting speed)
{
	uint32_t tmp;

	tmp = SDCARD_SPI->CR1;
	if (speed== INTERFACE_SLOW ) 
	{
		/* Set slow clock (100k-400k) */
		tmp = (( tmp & ~SPI_CR1_BR)|SPI_BaudRatePrescaler_256);
	} else 
	{
		/* Set fast clock (depends on the CSD) */
		tmp = (( tmp & ~SPI_CR1_BR)|SPI_BaudRatePrescaler_64);
	}
	SDCARD_SPI->CR1 = tmp;
}


/*-----------------------------------------------------------------------*/
/* Transmit/Receive a byte to MMC via SPI  (Platform dependent)          */
/*-----------------------------------------------------------------------*/
static uint8_t spi_rw(  uint8_t out )
{
	uint8_t result = 0;
	static  uint32_t  i;

	i = 10000;
	while(SPI_I2S_GetFlagStatus(SDCARD_SPI,SPI_I2S_FLAG_BSY)&&(i > 0))
	{
		i --;
	}
	
	/* Send byte through the SPI peripheral */
	SPI_I2S_SendData(SDCARD_SPI,out);
	i = 10000;
	while(!SPI_I2S_GetFlagStatus(SDCARD_SPI,SPI_I2S_FLAG_TXE)&&(i > 0))
	{
		i --;
	}
	i = 10000;
	while(!SPI_I2S_GetFlagStatus(SDCARD_SPI,SPI_I2S_FLAG_RXNE)&&(i > 0))
	{
		i --;
	}
	/* Wait to receive a byte */
	result = SPI_I2S_ReceiveData(SDCARD_SPI);

	/* Return the byte read from the SPI bus */
	return result;
}


/*-----------------------------------------------------------------------*/
/* Receive a byte from MMC via SPI  (Platform dependent)                 */
/*-----------------------------------------------------------------------*/
/* Receive a response when sent oxff*/
static uint8_t rcvr_spi (void)
{
	return spi_rw(0xff);
}

/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/
/* tra ve oxff neu card san sang, dua vao return de xac dinh trang thai cua card */

static uint8_t wait_ready (void)
{
	uint8_t res;
	static  uint32_t  i;
	i =82000;
	
	res = rcvr_spi(); /* Tranmit oxff and receive response by spi_rw (oxff) */
	do
	{
		res = rcvr_spi();
		i-- ;
	} while ((res != 0xFF) && i>0);
	
	return res;
}

/*-----------------------------------------------------------------------*/
/* Deselect the card and release SPI bus                                 */
/*-----------------------------------------------------------------------*/
static void release_spi (void)
{
	DESELECT(); /* CS= HIGHT*/
	rcvr_spi();   /* Need 8 cycle clocks to  succesively finish*/
}

/*-----------------------------------------------------------------------*/
/* Receive a data packet from MMC                                        */
/*-----------------------------------------------------------------------*/
static bool rcvr_datablock (
 	uint8_t *buff,			/* Data buffer to store received data */
	uint16_t  btr			/* Byte count (must be multiple of 4) */
)
{
	uint8_t token;
	static  uint32_t  i;

		i=9000;
	do {							/* Wait for data packet in timeout of 100ms */
		token = rcvr_spi();		/* Wait for valid response oxfe*/
		i--;
	} while ((token == 0xFF) && i>0); /*if escape, token = oxff if timer1 is out, else token=oxfe*/
	if(token != 0xFE) return false;	/* If not valid data token, return with error */
	do {							/* Receive the data block into buffer */
		rcvr_spi_m(buff++);
		rcvr_spi_m(buff++);
		rcvr_spi_m(buff++);
		rcvr_spi_m(buff++);
	} while (btr -= 4);

	rcvr_spi();						/* Discard CRC */
	rcvr_spi();

	return true;					/* Return with success */
}



/*-----------------------------------------------------------------------*/
/* Send a data packet to MMC                                             */
/*-----------------------------------------------------------------------*/
/* No use in my thesic */
static bool xmit_datablock (
	const uint8_t *buff,	/* 512 byte data block to be transmitted */
	uint8_t  token			/* Data/Stop token */
)
{
	uint8_t resp;
	uint8_t  wc ;
	if (wait_ready() != 0xFF) return false;

	xmit_spi(token);					/* transmit data token */

	/* Is data token */
	if (token != 0xFD) 
	{	
		wc = 0;
		do {							/* transmit the 512 byte data block to MMC */
			xmit_spi(*buff++);
			xmit_spi(*buff++);
		} while (--wc);

		xmit_spi(0xFF);					/* CRC (Dummy) */
		xmit_spi(0xFF);
		
		resp = rcvr_spi();				/* Receive data response */
		
		if ((resp & 0x1F) != 0x05)		/* If not accepted, return with error */
			return false;
	}						/* illegal command and in idle state*/

	return true;
}

/*-----------------------------------------------------------------------*/
/* Send a command packet to MMC                                          */
/*-----------------------------------------------------------------------*/

static uint8_t send_cmd (
	uint8_t cmd,		/* Command byte */
	uint32_t arg		/* Argument  usually is address*/
)
{
	uint8_t n, res;

	/* Select the card and wait for ready */
	DESELECT();
	SELECT();
	if (wait_ready() != 0xFF)  /* time out*/
		return 0xFF;

	/* ACMD<n> is the command sequense of CMD55-CMD<n> */
	if (cmd & 0x80) 
	{	
		cmd &= 0x7F;
		res = send_cmd(CMD55, 0);
		if (res > 1) return res;
	}
	
	/* Send command packet */
	xmit_spi(cmd);						/* Start + Command index */
	xmit_spi((uint8_t)(arg >> 24));		/* Argument[31..24] */
	xmit_spi((uint8_t)(arg >> 16));		/* Argument[23..16] */
	xmit_spi((uint8_t)(arg >> 8));			/* Argument[15..8] */
	xmit_spi((uint8_t)arg);				/* Argument[7..0] */
	
	n = 0x01;							/* Dummy CRC + Stop */
	
	if (cmd == CMD0) n = 0x95;			/* Valid CRC for CMD0(0) */
	if (cmd == CMD8) n = 0x87;			/* Valid CRC for CMD8(0x1AA) */
	
	xmit_spi(n);

	/* Receive command response */
	if (cmd == CMD12) rcvr_spi();		/* Skip a stuff byte when stop reading */

	n = 200;								/* Wait for a valid response in timeout of 10 attempts */
	do
	{
		res = rcvr_spi();
	} while ((res & 0x80) && --n); /* wait for valid respose*/

	return res;			/* Return with the response value */
}



/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize (uint8_t drv)		/* Physical drive number (0) */
{
	BYTE cmd, ty, ocr[4];
	uint32_t i,n;


	if (drv) return STA_NOINIT;					/* Supports only single drive */
	if (Stat & STA_NODISK) return Stat;			/* No card in the socket */

	DESELECT();									/* Force socket power on */
	
	interface_speed(INTERFACE_SLOW);
	
	for (n = 12; n; n--) rcvr_spi();			/* 80 dummy clocks */

	ty = 0;
	if (send_cmd(CMD0, 0) == 1) 				/* Enter Idle state */
	{			
		i = 8200;								/* Initialization timeout of 1000 msec */
		if (send_cmd(CMD8, 0x1AA) == 1) 		/* SDv2? */
		{	
			for (n = 0; n < 4; n++) ocr[n] = rcvr_spi();			/* Get trailing return value of R7 resp */

			if (ocr[2] == 0x01 && ocr[3] == 0xAA) 					/* The card can work at vdd range of 2.7-3.6V */
			{				
				i = 2000;											/* Initialization timeout of 1000 milliseconds */
				do
				{	
					i--;
					// Delay
					for(n=0;n<1000;n++) {};
				} while (i && send_cmd(ACMD41, 0x40000000));			/* Wait for leaving idle state (ACMD41 with HCS bit) */

				if (i && send_cmd(CMD58, 0) == 0) 						/* Check CCS bit in the OCR */
				{			
					for (n = 0; n < 4; n++) ocr[n] = rcvr_spi();
					ty = (ocr[0] & 0x40) ? CT_SD2|CT_BLOCK : CT_SD2;	/* SDv2 */
				}
			}
		} 
		else 									/* SDv1 or MMCv3 */
		{							
			if (send_cmd(ACMD41, 0) <= 1) 	
			{
				ty = CT_SD1; cmd = ACMD41;	/* SDv1 */
			} 
			else 
			{
				ty = CT_MMC; cmd = CMD1;	/* MMCv3 */
			}
			i = 8200;					/* Initialization timeout of 1000 milliseconds */
			do
			{	
				i--;
			} while (i && send_cmd(cmd, 0));		/* Wait for leaving idle state */

			if (!i || send_cmd(CMD16, 512) != 0)	/* Set read/write block length to 512 */
			ty = 0;
		}
	}
	
	CardType = ty;
	DESELECT();

	if (ty) 					/* Initialization succeded */
	{			
		Stat &= ~STA_NOINIT;	/* Clear STA_NOINIT */
		interface_speed(INTERFACE_FAST);
	} 
	else 						/* Initialization failed */
	{			
		Stat = STA_NOINIT;		/* Disk status */	
	}

	return Stat;
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

uint8_t disk_read (
	uint8_t drv,			/* Physical drive number (0) */
	uint8_t *buff,			/* Pointer to the data buffer to store read data */
	uint32_t  sector,		/* Start sector number (LBA) */
	uint8_t count			/* Sector count (1..255) */
)
{
	if (drv || !count) return RES_PARERR;
	
	if (Stat & STA_NOINIT) return RES_NOTRDY;

	if (!(CardType & CT_BLOCK)) sector *= 512;	/* Convert to byte address if needed */

	if (count == 1) 							/* Single block read */
	{	
		/* READ_SINGLE_BLOCK */
		if((send_cmd(CMD17, sector) == 0) && rcvr_datablock(buff, 512))
			count = 0;
	}
	else 									/* Multiple block read */
	{				
		if (send_cmd(CMD18, sector) == 0) 	/* READ_MULTIPLE_BLOCK */
		{	
			do 
			{
				if (!rcvr_datablock(buff, 512)) break;
				buff += 512;
			} while (--count);
			send_cmd(CMD12, 0);				/* STOP_TRANSMISSION */
		}
	}
	DESELECT();

	return count ? RES_ERROR : RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

uint8_t disk_write (
	uint8_t drv,			/* Physical drive number (0) */
	const uint8_t *buff,	/* Pointer to the data to be written */
	uint32_t sector,		/* Start sector number (LBA) */
	uint8_t count			/* Sector count (1..255) */
)
{	
	
	if (drv || !count) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;
	if (Stat & STA_PROTECT) return RES_WRPRT;

	if (!(CardType & CT_BLOCK)) sector *= 512;		/* Convert to byte address if needed */

	if (count == 1) 								/* Single block write */
	{	
		/* WRITE_BLOCK */
		if ((send_cmd(CMD24, sector) == 0) && xmit_datablock(buff, 0xFE))
			count = 0;
	}
	else 											/* Multiple block write */
	{	
		if (CardType & CT_SDC) send_cmd(ACMD23, count);
		/* WRITE_MULTIPLE_BLOCK */
		if (send_cmd(CMD25, sector) == 0) 
		{			
			do 
			{
				if (!xmit_datablock(buff, 0xFC)) break;
				buff += 512;
			} while (--count);
			if (!xmit_datablock(0, 0xFD))	/* STOP_TRAN token */
				count = 1;
		}
	}
	DESELECT();

	return count ? RES_ERROR : RES_OK;
		
}
/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	BYTE n, csd[16];
	WORD cs;


	if (disk_status(drv) & STA_NOINIT) return RES_NOTRDY;	/* Check if card is in the socket */

	res = RES_ERROR;
	switch (ctrl) 
	{
		case CTRL_SYNC :		/* Make sure that no pending write process */
			
				DESELECT();
				res = RES_OK;
				
			break;

		case GET_SECTOR_COUNT :	/* Get number of sectors on the disk (DWORD) */
			
			if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) 
			{
				/* SDC ver 2.00 */
				if ((csd[0] >> 6) == 1) 
				{	
					cs= csd[9] + ((WORD)csd[8] << 8) + 1;
					*(DWORD*)buff = (DWORD)cs << 10;
				} 
				else 									/* SDC ver 1.XX or MMC */
				{					
					n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
					cs = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
					*(DWORD*)buff = (DWORD)cs << (n - 9);
				}
				res = RES_OK;
			}
			
			break;

		case GET_BLOCK_SIZE :	/* Get erase block size in unit of sector (DWORD) */
			*(DWORD*)buff = 128;
			res = RES_OK;
			break;

		default:
			res = RES_PARERR;
	}

	DESELECT();

	return res;
}



/*-----------------------------------------------------------------------*/
/* This function is defined for only project compatibility               */

void disk_timerproc (void)
{
	/* Nothing to do */
}

/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE drv			/* Drive number (always 0) */
)
{
	DSTATUS s;
	BYTE d;


	if (drv) return STA_NOINIT;

	/* Check if the card is kept initialized */
	s = Stat;
	
	if (!(s & STA_NOINIT)) 
	{
		if (send_cmd(CMD13, 0))	/* Read card status */
			s = STA_NOINIT;
		
		rcvr_spi_m(&d);			/* Receive following half of R2 */
		DESELECT();
	}
	Stat = s;

	return s;
}

