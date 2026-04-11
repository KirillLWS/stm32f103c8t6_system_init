################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../SystemInit/system_stm32f1xx.c 

OBJS += \
./SystemInit/system_stm32f1xx.o 

C_DEPS += \
./SystemInit/system_stm32f1xx.d 


# Each subdirectory must supply rules for building sources it contributes
SystemInit/%.o SystemInit/%.su SystemInit/%.cyclo: ../SystemInit/%.c SystemInit/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DSTM32 -DSTM32F1 -DSTM32F103C8Tx -c -I../Inc -I"/home/lewars/git_repos/stm32/stm32f103c8t6_system_init/InitSystem/SystemInit" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-SystemInit

clean-SystemInit:
	-$(RM) ./SystemInit/system_stm32f1xx.cyclo ./SystemInit/system_stm32f1xx.d ./SystemInit/system_stm32f1xx.o ./SystemInit/system_stm32f1xx.su

.PHONY: clean-SystemInit

