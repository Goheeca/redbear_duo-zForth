/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */

/* Definitions of physical drive number for each drive */
#define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
#define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */


#include "spark_wiring_sflash.h"
#include "platform_config.h"

#include "Particle.h"

#define DEBUG_TRACE 0

static DSTATUS _stat = STA_NOINIT;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
#if DEBUG_TRACE
	Serial.printf("disk_status(pdrv = %u) = ", pdrv);
#endif
	DSTATUS stat = STA_NOINIT;

	if(pdrv == 0) {
		stat = _stat;
	}
#if DEBUG_TRACE
	Serial.printf("%u\r\n", stat);
#endif
	return stat;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
#if DEBUG_TRACE
	Serial.printf("disk_initialize(pdrv = %u) = ", pdrv);
#endif
	DSTATUS stat = STA_NOINIT;

	if(pdrv == 0) {
		/*if(_stat == STA_NOINIT) {
			_stat = sFLASH.selfTest();
			_stat = STA_NOINIT ? _stat : _stat;
		}*/
		_stat = 0; // Always ok
		stat = _stat;
	}
#if DEBUG_TRACE
	Serial.printf("%u\r\n", stat);
#endif
	return stat;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
#if DEBUG_TRACE
	Serial.printf("disk_read(pdrv = %u, buff = %p, sector = %u, count = %u) = ", pdrv, buff, sector, count);
#endif
	DRESULT res = RES_PARERR;

	if(pdrv == 0) {
		if ((count + sector) * FF_MAX_SS > SFLASH_RESERVED_ADDRESS) {
			return res;
		}

#if DEBUG_TRACE == 2
	Serial.printf("\r\n");
#endif
		for (UINT i = 0; i < count; i++) {
#if DEBUG_TRACE == 2
			Serial.printf("sFLASH.readBuffer(%p, 0x%x, %u)\r\n", (uint8_t *) &buff[i*FF_MAX_SS], (i+sector)*FF_MAX_SS, FF_MAX_SS);
#endif
			sFLASH.readBuffer((uint8_t *) &buff[i*FF_MAX_SS], (i+sector)*FF_MAX_SS, FF_MAX_SS);
		}

#if DEBUG_TRACE == 3
		for (UINT i = 0; i < FF_MAX_SS*count; i++) {
	        Serial.printf("%02X", buff[i]);
	    }
		Serial.printf("\r\n");
#endif
		res = RES_OK;
	}
#if DEBUG_TRACE
	Serial.printf("%u\r\n", res);
#endif
	return res;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
#if DEBUG_TRACE
	Serial.printf("disk_write(pdrv = %u, buff = %p, sector = %u, count = %u) = ", pdrv, buff, sector, count);
#endif
	DRESULT res = RES_PARERR;

	if(pdrv == 0) {
		if ((count + sector) * FF_MAX_SS > SFLASH_RESERVED_ADDRESS) {
			return res;
		}

#if DEBUG_TRACE == 2
		Serial.printf("\r\n");
#endif
#if DEBUG_TRACE == 3
		for (UINT i = 0; i < FF_MAX_SS*count; i++) {
	        Serial.printf("%02X", buff[i]);
	    }
		Serial.printf("\r\n");
#endif

		static uint8_t page_buff[sFLASH_PAGESIZE];
		static uint8_t sector_buff[FF_MAX_SS];
		for (UINT i = 0; i < count; i++) {
#if DEBUG_TRACE == 2
			Serial.printf("sFLASH.readBuffer(%p, 0x%x, %u)\r\n", sector_buff, (i+sector)*FF_MAX_SS, FF_MAX_SS);
#endif
			sFLASH.readBuffer(sector_buff, (i+sector)*FF_MAX_SS, FF_MAX_SS);
			bool erase = false;
			for(UINT j = 0; j < FF_MAX_SS; j++) {
				if ((sector_buff[j] & (const uint8_t) buff[i*FF_MAX_SS + j]) != (const uint8_t) buff[i*FF_MAX_SS + j]) {
					erase = true;
					break;
				}
			}
			if (erase) {
				DWORD aligned = (i+sector)*FF_MAX_SS / sFLASH_PAGESIZE * sFLASH_PAGESIZE;
#if DEBUG_TRACE == 2
				Serial.printf("sFLASH.readBuffer(%p, 0x%x, %u)\r\n", page_buff, aligned, sFLASH_PAGESIZE);
#endif
				sFLASH.readBuffer(page_buff, aligned, sFLASH_PAGESIZE);
#if DEBUG_TRACE == 2
				Serial.printf("sFLASH.eraseSector(0x%x)\r\n", aligned);
#endif
				sFLASH.eraseSector(aligned);
				delayMicroseconds(10000); // UNFORTUNATE
				//delay(100);
				for(UINT j = 0; j < FF_MAX_SS; j++) {
					page_buff[(i+sector)*FF_MAX_SS - aligned + j] = buff[i*FF_MAX_SS + j];
				}
#if DEBUG_TRACE == 2
				Serial.printf("sFLASH.writeBuffer(%p, 0x%x, %u)\r\n", page_buff, aligned, sFLASH_PAGESIZE);
#endif
				sFLASH.writeBuffer(page_buff, aligned, sFLASH_PAGESIZE);
			} else {
#if DEBUG_TRACE == 2
				Serial.printf("sFLASH.writeBuffer(%p, 0x%x, %u)\r\n", (const uint8_t *) &buff[(i+sector)*FF_MAX_SS], (i+sector)*FF_MAX_SS, FF_MAX_SS);
#endif
				sFLASH.writeBuffer((const uint8_t *) &buff[(i+sector)*FF_MAX_SS], (i+sector)*FF_MAX_SS, FF_MAX_SS);
			}
		}
		res = RES_OK;
	}
#if DEBUG_TRACE
	Serial.printf("%u\r\n", res);
#endif
	return res;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
#if DEBUG_TRACE
	const char *hint = "";
	switch (cmd) {
		case CTRL_SYNC:
			hint = "CTRL_SYNC";
			break;
		case GET_SECTOR_COUNT:
			hint = "GET_SECTOR_COUNT";
			break;
		case GET_SECTOR_SIZE:
			hint = "GET_SECTOR_SIZE";
			break;
		case GET_BLOCK_SIZE:
			hint = "GET_BLOCK_SIZE";
			break;
		case CTRL_TRIM:
			hint = "CTRL_TRIM";
			break;
	}
	Serial.printf("disk_ioctl(pdrv = %u, cmd = %u /* %s */, buff = %p) = ", pdrv, cmd, hint, buff);
#endif
	DRESULT res = RES_PARERR;
	WORD sector_size = FF_MAX_SS;//sFLASH_PAGESIZE;

	if(pdrv == 0) {
		switch (cmd) {
			case CTRL_SYNC:
				res = RES_OK;
				break;
			case GET_SECTOR_COUNT:
				*(DWORD*)buff = SFLASH_RESERVED_ADDRESS / sector_size;
				res = RES_OK;
				break;
			case GET_SECTOR_SIZE:
				*(WORD*)buff = sector_size;
				res = RES_OK;
				break;
			case GET_BLOCK_SIZE:
				*(DWORD*)buff = sFLASH_PAGESIZE / sector_size;
				res = RES_OK;
				break;
			case CTRL_TRIM:
				// Not implemented
				res = RES_OK;
				break;
		}
	}
#if DEBUG_TRACE
	Serial.printf("%u\r\n", res);
#endif
	return res;
}
