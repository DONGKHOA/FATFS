/*
 * bsp_sd_spi.c
 *
 *  Created on: Aug 5, 2024
 *      Author: dongkhoa
 */

/* Includes -----------------------------------------------------------------*/

#include "bsp_sd_def.h"
#include "bsp_sd_spi.h"
#include "ff.h"
#include "stm32f7xx.h"
#include "stm32f7xx_hal.h"
#include <stdbool.h>

#if (SD_USE_SPI == 1)

/* Extern variables ---------------------------------------------------------*/

extern SPI_HandleTypeDef hspi2;
#define HSPI_SDCARD &hspi2
#define SD_CS_GPIO_Port GPIOD
#define SD_CS_Pin GPIO_PIN_8

/* Private defines ----------------------------------------------------------*/

#define TRUE  1
#define FALSE 0

/* Private variables --------------------------------------------------------*/

static volatile BSP_SD_SPI_CardInfo cardInfo;

static volatile uint16_t Timer1, Timer2; /* 1ms Timer Counter */
static uint8_t           PowerFlag = 0;  /* Power flag */

/* Private function prototype -----------------------------------------------*/

static void    SELECT(void);
static void    DESELECT(void);
static void    SPI_TxByte(uint8_t data, uint32_t timeout);
static void    SPI_TxBuffer(uint8_t *buffer, uint16_t len, uint32_t timeout);
static uint8_t SPI_RxByte(uint32_t timeout);
static void    SPI_RxBytePtr(uint8_t *buff, uint32_t timeout);
static void    SD_PowerOn(void);
static void    SD_PowerOff(void);
static uint8_t SD_ReadyWait(void);
static uint8_t SD_CheckPower(void);
static bool    SD_RxDataBlock(uint32_t *buff, uint32_t len, uint32_t timeout);
static bool    SD_TxDataBlock(uint8_t *buff, BYTE token, uint32_t timeout);
static BYTE    SD_SendCmd(BYTE cmd, uint32_t arg, uint32_t timeout);

/* Private function prototype -----------------------------------------------*/

uint8_t BSP_SD_SPI_Init(void);
uint8_t BSP_SD_SPI_ITConfig(void);
uint8_t BSP_SD_SPI_ReadBlocks(uint32_t *pData,
                               uint32_t  ReadAddr,
                               uint32_t  NumOfBlocks,
                               uint32_t  Timeout);
uint8_t BSP_SD_SPI_WriteBlocks(uint32_t *pData,
                                uint32_t  WriteAddr,
                                uint32_t  NumOfBlocks,
                                uint32_t  Timeout);
uint8_t BSP_SD_SPI_ReadBlocks_DMA(uint32_t *pData,
                                   uint32_t  ReadAddr,
                                   uint32_t  NumOfBlocks);
uint8_t BSP_SD_SPI_WriteBlocks_DMA(uint32_t *pData,
                                    uint32_t  WriteAddr,
                                    uint32_t  NumOfBlocks);
uint8_t BSP_SD_SPI_Erase(uint32_t StartAddr, uint32_t EndAddr);
uint8_t BSP_SD_SPI_GetCardState(void);
void    BSP_SD_SPI_GetCardInfo(BSP_SD_SPI_CardInfo *CardInfo);
uint8_t BSP_SD_SPI_IsDetected(void);

 /* These functions can be modified in case the current settings (e.g. DMA
    stream) need to be changed for specific application needs */
void BSP_SD_SPI_AbortCallback(void);
void BSP_SD_SPI_WriteCpltCallback(void);
void BSP_SD_SPI_ReadCpltCallback(void);

/* ---------------------------------*
 * -------- STATIC FUNCTION --------*
 * ---------------------------------*/

/* SPI Chip Select */
static void SELECT(void)
{
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);
}

/* SPI Chip Deselect */
static void DESELECT(void)
{
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
}

/* SPI transmit a byte */
static void
SPI_TxByte (uint8_t data, uint32_t timeout)
{
  while (!__HAL_SPI_GET_FLAG(HSPI_SDCARD, SPI_FLAG_TXE))
    ;
  HAL_SPI_Transmit(HSPI_SDCARD, &data, 1, timeout);
}

/* SPI transmit buffer */
static void
SPI_TxBuffer (uint8_t *buffer, uint16_t len, uint32_t timeout)
{
  while (!__HAL_SPI_GET_FLAG(HSPI_SDCARD, SPI_FLAG_TXE))
    ;
  HAL_SPI_Transmit(HSPI_SDCARD, buffer, len, timeout);
}

/* SPI receive a byte */
static uint8_t
SPI_RxByte (uint32_t timeout)
{
  uint8_t dummy, data;
  dummy = 0xFF;

  while (!__HAL_SPI_GET_FLAG(HSPI_SDCARD, SPI_FLAG_TXE))
    ;
  HAL_SPI_TransmitReceive(HSPI_SDCARD, &dummy, &data, 1, timeout);

  return data;
}

/* SPI receive a byte via pointer */
static void
SPI_RxBytePtr (uint8_t *buff, uint32_t timeout)
{
  *buff = SPI_RxByte(timeout);
}

/* SD Ready */
/* wait SD ready */
static uint8_t
SD_ReadyWait (void)
{
  uint8_t res;

  /* timeout 500ms */
  Timer2 = 500;

  /* if SD goes ready, receives 0xFF */
  do
  {
    res = SPI_RxByte(Timer2);
  } while ((res != 0xFF) && Timer2);

  return res;
}

/* power on */
static void
SD_PowerOn (void)
{
  uint8_t  args[6];
  uint32_t cnt = 0x1FFF;

  /* transmit bytes to wake up */
  DESELECT();
  for (int i = 0; i < 10; i++)
  {
    SPI_TxByte(0xFF, 1000);
  }

  /* slave select */
  SELECT();

  /* make idle state */
  args[0] = CMD0; /* CMD0:GO_IDLE_STATE */
  args[1] = 0;
  args[2] = 0;
  args[3] = 0;
  args[4] = 0;
  args[5] = 0x95; /* CRC */

  SPI_TxBuffer(args, sizeof(args), 1000);

  /* wait response */
  while ((SPI_RxByte(1000) != 0x01) && cnt)
  {
    cnt--;
  }

  DESELECT();
  SPI_TxByte(0XFF, 1000);

  PowerFlag = 1;
}

/* power off */
static void
SD_PowerOff (void)
{
  PowerFlag = 0;
}

/* check power flag */
static uint8_t
SD_CheckPower (void)
{
  return PowerFlag;
}

/* receive data block */
static bool
SD_RxDataBlock (uint32_t *buff, uint32_t len, uint32_t timeout)
{
  uint8_t token;

  /* timeout 200ms */
  Timer1 = 200;

  /* loop until receive a response or timeout */
  do
  {
    token = SPI_RxByte(1000);
  } while ((token == 0xFF) && Timer1);

  /* invalid response */
  if (token != 0xFE)
  {
    return FALSE;
  }

  /* receive data */
  do
  {
    SPI_RxBytePtr((uint8_t *)buff++, 1000);
  } while (len--);

  /* discard CRC */
  SPI_RxByte(1000);
  SPI_RxByte(1000);

  return TRUE;
}

/* transmit data block */
static bool
SD_TxDataBlock (uint8_t *buff, BYTE token, uint32_t timeout)
{
  uint8_t resp;
  uint8_t i = 0;

  /* wait SD ready */
  if (SD_ReadyWait() != 0xFF)
  {
    return FALSE;
  }

  /* transmit token */
  SPI_TxByte(token, timeout);

  /* if it's not STOP token, transmit data */
  if (token != 0xFD)
  {
    SPI_TxBuffer((uint8_t *)buff, 512, timeout);

    /* discard CRC */
    SPI_RxByte(timeout);
    SPI_RxByte(timeout);

    /* receive response */
    while (i <= 64)
    {
      resp = SPI_RxByte(timeout);

      /* transmit 0x05 accepted */
      if ((resp & 0x1F) == 0x05)
      {
        break;
      }
      i++;
    }

    /* recv buffer clear */
    while (SPI_RxByte(timeout) == 0)
      ;
  }

  /* transmit 0x05 accepted */
  if ((resp & 0x1F) == 0x05)
  {
    return TRUE;
  }

  return FALSE;
}

/* transmit command */
static BYTE
SD_SendCmd (BYTE cmd, uint32_t arg, uint32_t timeout)
{
  uint8_t crc, res;

  /* wait SD ready */
  if (SD_ReadyWait() != 0xFF)
  {
    return 0xFF;
  }

  /* transmit command */
  SPI_TxByte(cmd, timeout);                  /* Command */
  SPI_TxByte((uint8_t)(arg >> 24), timeout); /* Argument[31..24] */
  SPI_TxByte((uint8_t)(arg >> 16), timeout); /* Argument[23..16] */
  SPI_TxByte((uint8_t)(arg >> 8), timeout);  /* Argument[15..8] */
  SPI_TxByte((uint8_t)arg, timeout);         /* Argument[7..0] */

  /* prepare CRC */
  if (cmd == CMD0)
  {
    crc = 0x95; /* CRC for CMD0(0) */
  }
  else if (cmd == CMD8)
  {
    crc = 0x87; /* CRC for CMD8(0x1AA) */
  }
  else
  {
    crc = 1;
  }

  /* transmit CRC */
  SPI_TxByte(crc, timeout);

  /* Skip a stuff byte when STOP_TRANSMISSION */
  if (cmd == CMD12)
  {
    SPI_RxByte(timeout);
  }

  /* receive response */
  uint8_t n = 10;
  do
  {
    res = SPI_RxByte(timeout);
  } while ((res & 0x80) && --n);

  return res;
}

/* ---------------------------------*
 * -------- GLOBAL FUNCTION --------*
 * ---------------------------------*/

uint8_t BSP_SD_SPI_Init(void)
{
	uint8_t n, type, ocr[4];

	/* SD Power On */
	SD_PowerOn();
	/* SPI Chip Select */
	SELECT();
	type = 0;

	/* Idle State Entry */
	if (SD_SendCmd(CMD0, 0, 1000) == 1)
	{
	/* Set the timer for 1 second */
	Timer1 = 100;

	/* SD Checking Interface Behavior Conditions */
	if (SD_SendCmd(CMD8, 0x1AA, 1000) == 1)
	{
	  /* SDC Ver2+ */
	  for (n = 0; n < 4; n++)
	  {
		ocr[n] = SPI_RxByte(1000);
	  }
	  if (ocr[2] == 0x01 && ocr[3] == 0xAA)
	  {
		/* 2.7-3.6V */
		do {
		  if (SD_SendCmd(CMD55, 0, 1000) <= 1 && SD_SendCmd(CMD41, 1UL << 30, 1000) == 0)
			break; /* ACMD41 with HCS bit */
		} while (Timer1);

		if (Timer1 && SD_SendCmd(CMD58, 0, 1000) == 0)
		{
		  /* Check CCS bit */
		  for (n = 0; n < 4; n++)
		  {
			ocr[n] = SPI_RxByte(1000);
		  }
		  type = (ocr[0] & 0x40) ? 6 : 2;
		}
	  }
	}
	else
	{
	  /* SDC Ver1 or MMC */
	  type = (SD_SendCmd(CMD55, 0, 1000) <= 1 && SD_SendCmd(CMD41, 0, 1000) <= 1) ? 2 : 1; /* SDC : MMC */
	  do {
		if (type == 2)
		{
		  if (SD_SendCmd(CMD55, 0, 1000) <= 1 && SD_SendCmd(CMD41, 0, 1000) == 0)
			break; /* ACMD41 */
		}
		else
		{
		  if (SD_SendCmd(CMD1, 0,  1000) == 0)
			break; /* CMD1 */
		}
	  } while (Timer1);

	  if (!Timer1 || SD_SendCmd(CMD16, 512, 1000) != 0)
	  {
		/* Select Block Length */
		type = 0;
	  }
	}
	}

	cardInfo.CardType = type;

	DESELECT();

	SPI_RxByte(1000); /* Idle State Transitions (Release DO) */

	if (type)
	{
		// Get information
	    uint8_t n, csd[16];
	    uint32_t csize;
		if ((SD_SendCmd(CMD9, 0, 1000) == 0) && SD_RxDataBlock((uint32_t *)csd, 16, 1000))
		{
			if ((csd[0] >> 6) == 1)
			{
				/* SDC V2 */
				csize = csd[9] + ((WORD) csd[8] << 8) + 1;
				cardInfo.LogBlockNbr = csize << 10;
			}
			else
			{
				/* MMC or SDC V1 */
				n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
				csize = (csd[8] >> 6) + (csd[7] << 2) + ((csd[6] & 3) << 10) + 1;
				cardInfo.LogBlockNbr = csize << (n - 9);
			}
			cardInfo.LogBlockSize = 512;
		}
		return MSD_OK;
	}
	else
	{
		/* Initialization failed */
		SD_PowerOff();
		return MSD_ERROR;
	}
}

uint8_t BSP_SD_SPI_ITConfig(void)
{
	return 0;
}

uint8_t BSP_SD_SPI_ReadBlocks(uint32_t *pData,
                               uint32_t  ReadAddr,
                               uint32_t  NumOfBlocks,
                               uint32_t  Timeout)
{

	  SELECT();

	  if (NumOfBlocks == 1)
	  {
	    /* 싱글 블록 읽기 */
	    if ((SD_SendCmd(CMD17, ReadAddr, Timeout) == 0) && SD_RxDataBlock(pData, 512, Timeout))
	    	NumOfBlocks = 0;
	  }
	  else
	  {
	    /* 다중 블록 읽기 */
	    if (SD_SendCmd(CMD18, ReadAddr, Timeout) == 0)
	    {
	      do {
	        if (!SD_RxDataBlock(pData, 512, Timeout))
	          break;

	        pData += 512;
	      } while (--NumOfBlocks);

	      /* STOP_TRANSMISSION, 모든 블럭을 다 읽은 후, 전송 중지 요청 */
	      SD_SendCmd(CMD12, 0, Timeout);
	    }
	  }

	  DESELECT();
	  SPI_RxByte(Timeout); /* Idle 상태(Release DO) */

	  return NumOfBlocks ? MSD_ERROR : MSD_OK;
}

uint8_t BSP_SD_SPI_WriteBlocks(uint32_t *pData,
                                uint32_t  WriteAddr,
                                uint32_t  NumOfBlocks,
                                uint32_t  Timeout)
{
	if (!(cardInfo.CardType & CT_SD2))
	  {
	    WriteAddr *= 512;
	  }

	  SELECT();

	  if (NumOfBlocks == 1)
	  {
	    /* 싱글 블록 쓰기 */
	    if ((SD_SendCmd(CMD24, WriteAddr, Timeout) == 0) && SD_TxDataBlock((uint8_t *)pData, 0xFE, Timeout))
	    	NumOfBlocks = 0;
	  }
	  else
	  {
		  /* WRITE_MULTIPLE_BLOCK */
	    if (cardInfo.CardType & CT_SD1)
	    {
	    	SD_SendCmd(CMD55, 0, Timeout);
	    	SD_SendCmd(CMD23, NumOfBlocks, Timeout); /* ACMD23 */
	    }
	    if (SD_SendCmd(CMD25, WriteAddr, Timeout) == 0)
	        {
	          do
	          {
	            if (!SD_TxDataBlock((uint8_t *)pData, 0xFC, Timeout))
	            {
	              break;
	            }
	            pData += 512;
	          } while (--NumOfBlocks);

	          /* STOP_TRAN token */
	          if (!SD_TxDataBlock(0, 0xFD, Timeout))
	          {
	            NumOfBlocks = 1;
	          }
	        }
	      }

	  DESELECT();
	  SPI_RxByte(1000);

	  return NumOfBlocks ? MSD_ERROR : MSD_OK;
}

uint8_t BSP_SD_SPI_ReadBlocks_DMA(uint32_t *pData,
                                   uint32_t  ReadAddr,
                                   uint32_t  NumOfBlocks)
{
	return 0;
}
uint8_t BSP_SD_SPI_WriteBlocks_DMA(uint32_t *pData,
                                    uint32_t  WriteAddr,
                                    uint32_t  NumOfBlocks)
{
	return 0;
}
/////////////////////////////////////////////////////////////
uint8_t BSP_SD_SPI_ReadBlocks_DMA(uint32_t *pData,
                                  uint32_t  ReadAddr,
                                  uint32_t  NumOfBlocks);
uint8_t BSP_SD_SPI_WriteBlocks_DMA(uint32_t *pData,
                                   uint32_t  WriteAddr,
                                   uint32_t  NumOfBlocks);
uint8_t BSP_SD_SPI_Erase(uint32_t StartAddr, uint32_t EndAddr);
uint8_t BSP_SD_SPI_GetCardState(void);

/**
 * @brief Retrieves information about the SD card and populates the provided
 *        `BSP_SD_SPI_CardInfo` structure.
 *
 * This function fills the `CardInfo` structure with details about the SD card,
 * including the card type, the number of logical blocks, and the size of each
 * logical block. The information is fetched from a global or internal
 * `cardInfo` object that contains the SD card's current configuration.
 *
 * @param CardInfo Pointer to a `BSP_SD_SPI_CardInfo` structure to be filled
 *                 with the card information.
 */
void
BSP_SD_SPI_GetCardInfo (BSP_SD_SPI_CardInfo *CardInfo)
{
  CardInfo->CardType     = cardInfo.CardType;
  CardInfo->LogBlockNbr  = cardInfo.LogBlockNbr;
  CardInfo->LogBlockSize = cardInfo.LogBlockSize;
}

#endif
