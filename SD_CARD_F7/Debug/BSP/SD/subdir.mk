################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../BSP/SD/SD.c 

OBJS += \
./BSP/SD/SD.o 

C_DEPS += \
./BSP/SD/SD.d 


# Each subdirectory must supply rules for building sources it contributes
BSP/SD/%.o BSP/SD/%.su BSP/SD/%.cyclo: ../BSP/SD/%.c BSP/SD/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DSTM32F746xx -DUSE_FULL_LL_DRIVER -DUSE_HAL_DRIVER -c -I../Core/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/dongkhoa/STM32CubeIDE/workspace_1.16.0/SD_CARD_F7/BSP/SDIO" -I../FATFS/Target -I../FATFS/App -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FatFs/src -I"C:/Users/dongkhoa/STM32CubeIDE/workspace_1.16.0/SD_CARD_F7/BSP/myDiskio" -I"C:/Users/dongkhoa/STM32CubeIDE/workspace_1.16.0/SD_CARD_F7/BSP/SD" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-BSP-2f-SD

clean-BSP-2f-SD:
	-$(RM) ./BSP/SD/SD.cyclo ./BSP/SD/SD.d ./BSP/SD/SD.o ./BSP/SD/SD.su

.PHONY: clean-BSP-2f-SD

