/*
 * bsp_sd_sdmmc.h
 *
 *  Created on: Aug 5, 2024
 *      Author: dongkhoa
 */

#ifndef SD_BSP_SD_SDMMCSDMMC_H_
#define SD_BSP_SD_SDMMCSDMMC_H_

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"

/* Exported types --------------------------------------------------------*/
/**
  * @brief SD Card information structure
  */
#define BSP_SD_SDMMC_CardInfo HAL_SD_CardInfoTypeDef

/**
* @brief  SD status structure definition
*/
#define MSD_OK                   ((uint8_t)0x00)
#define MSD_ERROR                ((uint8_t)0x01)
#define MSD_ERROR_SD_NOT_PRESENT ((uint8_t)0x02)

/**
* @brief  SD transfer state definition
*/
#define SD_TRANSFER_OK   ((uint8_t)0x00)
#define SD_TRANSFER_BUSY ((uint8_t)0x01)

#define SD_PRESENT     ((uint8_t)0x01)
#define SD_NOT_PRESENT ((uint8_t)0x00)
#define SD_DATATIMEOUT ((uint32_t)100000000)

/* Puplic functions --------------------------------------------------------*/
uint8_t BSP_SD_SDMMC_Init(void);
uint8_t BSP_SD_SDMMC_ITConfig(void);
uint8_t BSP_SD_SDMMC_ReadBlocks(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks, uint32_t Timeout);
uint8_t BSP_SD_SDMMC_WriteBlocks(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks, uint32_t Timeout);
uint8_t BSP_SD_SDMMC_ReadBlocks_DMA(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks);
uint8_t BSP_SD_SDMMC_WriteBlocks_DMA(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks);
uint8_t BSP_SD_SDMMC_Erase(uint32_t StartAddr, uint32_t EndAddr);
uint8_t BSP_SD_SDMMC_GetCardState(void);
void    BSP_SD_SDMMC_GetCardInfo(BSP_SD_SDMMC_CardInfo *CardInfo);
uint8_t BSP_SD_SDMMC_IsDetected(void);

/* These functions can be modified in case the current settings (e.g. DMA stream)
   need to be changed for specific application needs */
void    BSP_SD_SDMMC_AbortCallback(void);
void    BSP_SD_SDMMC_WriteCpltCallback(void);
void    BSP_SD_SDMMC_ReadCpltCallback(void);

#ifdef __cplusplus
}
#endif

#endif /* SD_BSP_SD_SDMMCSDMMC_H_ */
