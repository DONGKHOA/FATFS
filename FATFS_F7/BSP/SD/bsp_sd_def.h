/*
 * bsp_sd_def.h
 *
 *  Created on: Aug 6, 2024
 *      Author: dongkhoa
 */

#ifndef SD_BSP_SD_DEF_H_
#define SD_BSP_SD_DEF_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* Public defines -----------------------------------------------------------*/

  // Config interface to SD-Card or uSD
#define SD_USE_SDMMC 0
#define SD_USE_SDIO  0
#define SD_USE_SPI   1
#define SD_USE_DMA   1

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

/* Includes -----------------------------------------------------------------*/

#if (SD_USE_SDMMC == 1) || (SD_USE_SDIO == 1)

#include "bsp_sd_sdmmc.h"

#elif (SD_USE_SPI == 1)

#include "bsp_sd_spi.h"

#endif

#ifdef __cplusplus
}
#endif

#endif /* SD_BSP_SD_DEF_H_ */
