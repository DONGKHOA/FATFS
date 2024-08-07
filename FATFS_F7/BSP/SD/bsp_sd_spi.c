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
#include <stdbool.h>

/* Extern variables ---------------------------------------------------------*/

extern SPI_HandleTypeDef hspi2;
#define HSPI_SDCARD &hspi2
#define SD_CS_PORT  GPIOI
#define SD_CS_PIN   GPIO_PIN_0

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

/* Public function ----------------------------------------------------------*/
uint8_t
BSP_SD_SPI_Init (void)
{
  uint8_t n, type, ocr[4];

  /* power on */
  SD_PowerOn();

  /* slave select */
  SELECT();

  /* check disk type */
  type = 0;

  /* send GO_IDLE_STATE command */
  if (SD_SendCmd(CMD0, 0, 1000) == 1)
  {
    /* timeout 1 sec */
    Timer1 = 1000;

    /* SDC V2+ accept CMD8 command, http://elm-chan.org/docs/mmc/mmc_e.html */
    if (SD_SendCmd(CMD8, 0x1AA, 1000) == 1)
    {
      /* operation condition register */
      for (n = 0; n < 4; n++)
      {
        ocr[n] = SPI_RxByte(1000);
      }

      /* voltage range 2.7-3.6V */
      if (ocr[2] == 0x01 && ocr[3] == 0xAA)
      {
        /* ACMD41 with HCS bit */
        do
        {
          if (SD_SendCmd(CMD55, 0, 1000) <= 1
              && SD_SendCmd(ACMD41, 1UL << 30, 1000) == 0)
          {
            break;
          }
        } while (Timer1);

        /* READ_OCR */
        if (Timer1 && SD_SendCmd(CMD58, 0, 1000) == 0)
        {
          /* Check CCS bit */
          for (n = 0; n < 4; n++)
          {
            ocr[n] = SPI_RxByte(1000);
          }

          /* SDv2 (HC or SC) */
          type = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;
        }
      }
    }
    else
    {
      /* SDC V1 or MMC */
      type = (SD_SendCmd(CMD55, 0, 1000) <= 1
              && SD_SendCmd(ACMD41, 0, 1000) <= 1)
                 ? CT_SD1
                 : CT_MMC;

      do
      {
        if (type == CT_SD1)
        {
          if (SD_SendCmd(CMD55, 0, 1000) <= 1
              && SD_SendCmd(ACMD41, 0, 1000) == 0)
          {
            break; /* ACMD41 */
          }
        }
        else
        {
          if (SD_SendCmd(CMD1, 0, 1000) == 0)
          {
            break; /* CMD1 */
          }
        }

      } while (Timer1);

      /* SET_BLOCKLEN */
      if (!Timer1 || SD_SendCmd(CMD16, 512, 1000) != 0)
      {
        type = 0;
      }
    }
  }

  cardInfo.CardType = type;

  /* Idle */
  DESELECT();
  SPI_RxByte(1000);

  /* Clear STA_NOINIT */
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

uint8_t
BSP_SD_SPI_ITConfig (void)
{
  return (uint8_t)0;
}

/**
 * @brief Reads multiple blocks of data from an SD card via SPI.
 *
 * This function initiates a read operation from the SD card for a specified
 * number of blocks starting from a given address. It handles both single and
 * multiple block reads and includes timeout handling to manage delays.
 *
 * @param pData Pointer to the buffer where the read data will be stored.
 * @param ReadAddr The starting address from which to read the data blocks.
 * @param NumOfBlocks The number of data blocks to read.
 * @param Timeout The maximum time to wait for the read operation to complete.
 *
 * @return uint8_t Status of the read operation:
 *         - `MSD_OK` if the read operation is successful.
 *         - `MSD_ERROR` if an error occurs during the read operation.
 */
uint8_t
BSP_SD_SPI_ReadBlocks (uint32_t *pData,
                       uint32_t  ReadAddr,
                       uint32_t  NumOfBlocks,
                       uint32_t  Timeout)
{
  SELECT();

  if (NumOfBlocks == 1)
  {
    /* READ_SINGLE_BLOCK */
    if ((SD_SendCmd(CMD17, ReadAddr, Timeout) == 0)
        && SD_RxDataBlock(pData, 512, Timeout))
    {
      NumOfBlocks = 0;
    }
  }
  else
  {
    /* READ_MULTIPLE_BLOCK */
    if (SD_SendCmd(CMD18, ReadAddr, Timeout) == 0)
    {
      do
      {
        if (!SD_RxDataBlock(pData, 512, Timeout))
        {
          break;
        }
        pData += 512;
      } while (--NumOfBlocks);

      /* STOP_TRANSMISSION */
      SD_SendCmd(CMD12, 0, Timeout);
    }
  }

  /* Idle */
  DESELECT();
  SPI_RxByte(Timeout);

  return NumOfBlocks ? MSD_ERROR : MSD_OK;
}

/**
 * @brief Writes multiple blocks of data to an SD card via SPI.
 *
 * This function initiates a write operation to the SD card for a specified
 * number of blocks starting from a given address. It handles both single and
 * multiple block writes and includes timeout handling to manage delays. It also
 * adjusts the address if necessary depending on the SD card type.
 *
 * @param pData Pointer to the buffer containing the data to be written to the
 * SD card.
 * @param WriteAddr The starting address where the data blocks will be written.
 * @param NumOfBlocks The number of data blocks to write.
 * @param Timeout The maximum time to wait for the write operation to complete.
 *
 * @return uint8_t Status of the write operation:
 *         - `MSD_OK` if the write operation is successful.
 *         - `MSD_ERROR` if an error occurs during the write operation.
 */
uint8_t
BSP_SD_SPI_WriteBlocks (uint32_t *pData,
                        uint32_t  WriteAddr,
                        uint32_t  NumOfBlocks,
                        uint32_t  Timeout)
{
  /* Convert to byte address if not SD2 card */
  if (!(cardInfo.CardType & CT_SD2))
  {
    WriteAddr *= 512;
  }

  SELECT();

  if (NumOfBlocks == 1)
  {
    /* WRITE_BLOCK */
    if ((SD_SendCmd(CMD24, WriteAddr, Timeout) == 0)
        && SD_TxDataBlock((uint8_t *)pData, 0xFE, Timeout))
    {
      NumOfBlocks = 0;
    }
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

  /* Idle */
  DESELECT();
  SPI_RxByte(Timeout);

  return NumOfBlocks ? MSD_ERROR : MSD_OK;
}

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

uint8_t BSP_SD_SPI_IsDetected(void);

/* These functions can be modified in case the current settings (e.g. DMA
   stream) need to be changed for specific application needs */
void BSP_SD_SPI_AbortCallback(void);
void BSP_SD_SPI_WriteCpltCallback(void);
void BSP_SD_SPI_ReadCpltCallback(void);

/* Private function ---------------------------------------------------------*/

/* slave select */
static void
SELECT (void)
{
  HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_RESET);
  HAL_Delay(1);
}

/* slave deselect */
static void
DESELECT (void)
{
  HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET);
  HAL_Delay(1);
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

/***************************************
 * SD functions
 **************************************/

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
