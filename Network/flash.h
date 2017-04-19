#ifndef __FLASH_H__
#define __FLASH_H__

#define FLASH_WRITE_BLOCK		64		// bytes

BYTE flashReadByte(u32 addr);
void flashReadBlock(u32 addr,u8* buff);
void flashEraseBlock(u32 addr);
void flashWriteBlock(u32 addr,u8* buff);

#endif
