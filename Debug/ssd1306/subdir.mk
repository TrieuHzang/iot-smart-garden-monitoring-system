################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../stm32-ssd1306-master/stm32-ssd1306-master/ssd1306/ssd1306.c \
../stm32-ssd1306-master/stm32-ssd1306-master/ssd1306/ssd1306_fonts.c \
../stm32-ssd1306-master/stm32-ssd1306-master/ssd1306/ssd1306_tests.c 

OBJS += \
./ssd1306/ssd1306.o \
./ssd1306/ssd1306_fonts.o \
./ssd1306/ssd1306_tests.o 

C_DEPS += \
./ssd1306/ssd1306.d \
./ssd1306/ssd1306_fonts.d \
./ssd1306/ssd1306_tests.d 


# Each subdirectory must supply rules for building sources it contributes
ssd1306/ssd1306.o: D:/FREERTOS_REAL/stm32-ssd1306-master/stm32-ssd1306-master/ssd1306/ssd1306.c ssd1306/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"D:/FREERTOS_REAL/Middlewares" -I"D:/FREERTOS_REAL/Drivers" -I"D:/FREERTOS_REAL/Delay timer" -I"D:/FREERTOS_REAL/DHT" -I"D:/FREERTOS_REAL/stm32-ssd1306-master/stm32-ssd1306-master/ssd1306" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
ssd1306/ssd1306_fonts.o: D:/FREERTOS_REAL/stm32-ssd1306-master/stm32-ssd1306-master/ssd1306/ssd1306_fonts.c ssd1306/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"D:/FREERTOS_REAL/Middlewares" -I"D:/FREERTOS_REAL/Drivers" -I"D:/FREERTOS_REAL/Delay timer" -I"D:/FREERTOS_REAL/DHT" -I"D:/FREERTOS_REAL/stm32-ssd1306-master/stm32-ssd1306-master/ssd1306" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
ssd1306/ssd1306_tests.o: D:/FREERTOS_REAL/stm32-ssd1306-master/stm32-ssd1306-master/ssd1306/ssd1306_tests.c ssd1306/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I"D:/FREERTOS_REAL/Middlewares" -I"D:/FREERTOS_REAL/Drivers" -I"D:/FREERTOS_REAL/Delay timer" -I"D:/FREERTOS_REAL/DHT" -I"D:/FREERTOS_REAL/stm32-ssd1306-master/stm32-ssd1306-master/ssd1306" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-ssd1306

clean-ssd1306:
	-$(RM) ./ssd1306/ssd1306.cyclo ./ssd1306/ssd1306.d ./ssd1306/ssd1306.o ./ssd1306/ssd1306.su ./ssd1306/ssd1306_fonts.cyclo ./ssd1306/ssd1306_fonts.d ./ssd1306/ssd1306_fonts.o ./ssd1306/ssd1306_fonts.su ./ssd1306/ssd1306_tests.cyclo ./ssd1306/ssd1306_tests.d ./ssd1306/ssd1306_tests.o ./ssd1306/ssd1306_tests.su

.PHONY: clean-ssd1306

