/*
 * bsp_sd_spi.h
 *
 *  Created on: Aug 5, 2024
 *      Author: dongkhoa
 */

#ifndef SD_BSP_SD_SPI_H_
#define SD_BSP_SD_SPI_H_

#ifdef __cplusplus
extern "C"
{
#endif

  /* Includes
   * -----------------------------------------------------------------*/

#include "stm32f7xx_hal.h"

  /* Public typedef
   * -----------------------------------------------------------*/

  typedef struct _BSP_SD_SPI_CardInfo
  {
    uint32_t CardType;     /*!< Specifies the card Type */
    uint32_t LogBlockNbr;  /*!< Specifies the Card logical Capacity in blocks */
    uint32_t LogBlockSize; /*!< Specifies logical block size in bytes */
  } BSP_SD_SPI_CardInfo;

/* Public defines -----------------------------------------------------------*/

/* MMC/SD command */
#define CMD0   (0)         /* GO_IDLE_STATE */
#define CMD1   (1)         /* SEND_OP_COND (MMC) */
#define ACMD41 (0x80 + 41) /* SEND_OP_COND (SDC) */
#define CMD8   (8)         /* SEND_IF_COND */
#define CMD9   (9)         /* SEND_CSD */
#define CMD10  (10)        /* SEND_CID */
#define CMD12  (12)        /* STOP_TRANSMISSION */
#define ACMD13 (0x80 + 13) /* SD_STATUS (SDC) */
#define CMD16  (16)        /* SET_BLOCKLEN */
#define CMD17  (17)        /* READ_SINGLE_BLOCK */
#define CMD18  (18)        /* READ_MULTIPLE_BLOCK */
#define CMD23  (23)        /* SET_BLOCK_COUNT (MMC) */
#define ACMD23 (0x80 + 23) /* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24  (24)        /* WRITE_BLOCK */
#define CMD25  (25)        /* WRITE_MULTIPLE_BLOCK */
#define CMD32  (32)        /* ERASE_ER_BLK_START */
#define CMD33  (33)        /* ERASE_ER_BLK_END */
#define CMD38  (38)        /* ERASE */
#define CMD55  (55)        /* APP_CMD */
#define CMD58  (58)        /* READ_OCR */

/* MMC card type flags (MMC_GET_TYPE) */
#define CT_MMC   0x01 /* MMC ver 3 */
#define CT_SD1   0x02 /* SD ver 1 */
#define CT_SD2   0x04 /* SD ver 2 */
#define CT_SDC   0x06 /* SD */
#define CT_BLOCK 0x08 /* Block addressing */

  /* Puplic functions -------------------------------------------------------*/
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

#ifdef __cplusplus
}
#endif

#endif /* SD_BSP_SD_SPI_H_ */
