/**
  * @brief Define to prevent recursive inclusion
  */
#ifndef __SYSTEM_STM32F10X_H
#define __SYSTEM_STM32F10X_H

#include <stm32f103xb.h>
#include <stdbool.h>
#include <stdint.h>

/* System defines */
#define HSE_VALUE               8000000U
#define HSI_VALUE               8000000U
#define ADC_MAX_FREQ_HZ   		14000000UL
#define APB1_MAX_FREQ_HZ  		36000000UL
#define HCLK_MAX_FREQ_HZ 		72000000UL
#define APB2_MAX_FREQ_HZ 		72000000UL
#define AHB_MAX_FREQ_HZ  		72000000UL
#define CHECK_TIMEOUT_US 		3000000UL	/* 3 микросекунды */

/* Структура ошибок и отказов */
typedef struct
{
	bool HCLK_Fail;
	bool AHB_Fail;
	bool APB1_Fail;
	bool APB2_Fail;
	bool ADC_Fail;
	bool LSI_Fail;
	bool InitSysTick_Fail;
	bool allIsOk;
}InitErrors_t;

extern uint32_t SystemCoreClock;
extern InitErrors_t InitErrors;

void SystemInit(void);


#endif /*__SYSTEM_STM32F10X_H */

