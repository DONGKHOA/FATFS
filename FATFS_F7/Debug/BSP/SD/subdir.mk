################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../BSP/SD/bsp_sd_sdmmc.c \
../BSP/SD/bsp_sd_spi.c 

OBJS += \
./BSP/SD/bsp_sd_sdmmc.o \
./BSP/SD/bsp_sd_spi.o 

C_DEPS += \
./BSP/SD/bsp_sd_sdmmc.d \
./BSP/SD/bsp_sd_spi.d 


# Each subdirectory must supply rules for building sources it contributes
BSP/SD/%.o BSP/SD/%.su BSP/SD/%.cyclo: ../BSP/SD/%.c BSP/SD/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F746xx -c -I../Core/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/PhamTuan/STM32CubeIDE/workspace_1.16.0/USB_MSC-uSD/FATFS_F7/FATFS" -I"C:/Users/PhamTuan/STM32CubeIDE/workspace_1.16.0/USB_MSC-uSD/FATFS_F7/BSP/SD" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-BSP-2f-SD

clean-BSP-2f-SD:
	-$(RM) ./BSP/SD/bsp_sd_sdmmc.cyclo ./BSP/SD/bsp_sd_sdmmc.d ./BSP/SD/bsp_sd_sdmmc.o ./BSP/SD/bsp_sd_sdmmc.su ./BSP/SD/bsp_sd_spi.cyclo ./BSP/SD/bsp_sd_spi.d ./BSP/SD/bsp_sd_spi.o ./BSP/SD/bsp_sd_spi.su

.PHONY: clean-BSP-2f-SD

