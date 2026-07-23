################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Delay\ timer/delay_timer.c 

OBJS += \
./Delay\ timer/delay_timer.o 

C_DEPS += \
./Delay\ timer/delay_timer.d 


# Each subdirectory must supply rules for building sources it contributes
Delay\ timer/delay_timer.o: ../Delay\ timer/delay_timer.c Delay\ timer/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"D:/FREERTOS_REAL/Middlewares" -I"D:/FREERTOS_REAL/Drivers" -I"D:/FREERTOS_REAL/Delay timer" -I"D:/FREERTOS_REAL/DHT" -I"D:/FREERTOS_REAL/stm32-ssd1306-master/stm32-ssd1306-master/ssd1306" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"Delay timer/delay_timer.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Delay-20-timer

clean-Delay-20-timer:
	-$(RM) ./Delay\ timer/delay_timer.cyclo ./Delay\ timer/delay_timer.d ./Delay\ timer/delay_timer.o ./Delay\ timer/delay_timer.su

.PHONY: clean-Delay-20-timer

