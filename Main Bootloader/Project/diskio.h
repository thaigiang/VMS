

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
/*-----------------------------------------------------------------------
/  Low level disk interface modlue include file  R0.07   (C)ChaN, 2009
/-----------------------------------------------------------------------*/

#ifndef _DISKIO

#include <stdint.h>
#include "typedef.h"

/* Status of Disk Functions */
typedef BYTE	DSTATUS;

/* Results of Disk Functions */
typedef enum {
	RES_OK = 0,     /* 0: Successful */
	RES_ERROR,      /* 1: R/W Error */
	RES_WRPRT,      /* 2: Write Protected */
	RES_NOTRDY,     /* 3: Not Ready */
	RES_PARERR      /* 4: Invalid Parameter */
} DRESULT;



/* Disk Status Bits (DSTATUS) */

#define STA_NOINIT		0x01	/* Drive not initialized */
#define STA_NODISK		0x02	/* No medium in the drive */
#define STA_PROTECT		0x04	/* Write protected */


/* Command code for disk_ioctrl() */

/* Generic ioctl command (defined for FatFs) */
#define CTRL_SYNC			0	/* Flush disk cache (for write functions) */
#define GET_SECTOR_COUNT	1	/* Get media size (for only f_mkfs()) */
#define GET_SECTOR_SIZE		2	/* Get sector size (for multiple sector size (_MAX_SS >= 1024)) */
#define GET_BLOCK_SIZE		3	/* Get erase block size (for only f_mkfs()) */

/* Generic ioctl command */
#define CTRL_POWER			4	/* Get/Set power status */
#define CTRL_LOCK			5	/* Lock/Unlock media removal */
#define CTRL_EJECT			6	/* Eject media */

/* MMC/SDC specific ioctl command */
#define MMC_GET_TYPE		10	/* Get card type */
#define MMC_GET_CSD			11	/* Get CSD */
#define MMC_GET_CID			12	/* Get CID */
#define MMC_GET_OCR			13	/* Get OCR */
#define MMC_GET_SDSTAT		14	/* Get SD status */

/* ATA/CF specific ioctl command */
#define ATA_GET_REV			20	/* Get F/W revision */
#define ATA_GET_MODEL		21	/* Get model name */
#define ATA_GET_SN			22	/* Get serial number */

/* NAND specific ioctl command */
#define NAND_FORMAT			30	/* Create physical format */
#define NAND_ERASE			31	/* Force erased a block */


/* MMC/SDC card type definitions (CardType) */

#define CT_MMC				0x01
#define CT_SD1				0x02
#define CT_SD2				0x04
#define CT_SDC				(CT_SD1|CT_SD2)
#define CT_BLOCK			0x08

/*---------------------------------------*/
/* Prototypes for disk control functions */
void power (void);
DSTATUS disk_initialize (uint8_t dr);
uint8_t disk_read(uint8_t dr , uint8_t *buff, uint32_t sector, uint8_t count);
uint8_t disk_write(uint8_t dr, const uint8_t *buff, uint32_t sector, uint8_t count);

DRESULT disk_ioctl (BYTE, BYTE, void*);
void	disk_timerproc (void);
DSTATUS disk_status (BYTE);

#define _DISKIO
#endif
