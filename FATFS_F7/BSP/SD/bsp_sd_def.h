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



/* Includes -----------------------------------------------------------------*/


#ifdef __cplusplus
}
#endif

#endif /* SD_BSP_SD_DEF_H_ */
