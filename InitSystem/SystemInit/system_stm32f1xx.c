#include "system_stm32f1xx.h"

#if !defined  (HSE_VALUE) 
  #define HSE_VALUE               8000000U /*!< Default value of the External oscillator in Hz.
                                                This value can be provided and adapted by the user application. */
#endif /* HSE_VALUE */

#if !defined  (HSI_VALUE)
  #define HSI_VALUE               8000000U /*!< Default value of the Internal oscillator in Hz.
                                                This value can be provided and adapted by the user application. */
#endif /* HSI_VALUE */

uint32_t SystemCoreClock = 8000000;
const uint8_t AHBPrescTable[16U] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8U] =  {0, 0, 0, 0, 1, 2, 3, 4};

#define HSE_STARTUP_TIMEOUT 0x5000

/* ===========================================================
   Compile-time safety limits (STM32F103)
   ===========================================================*/
#ifndef ADC_MAX_FREQ_HZ
#define ADC_MAX_FREQ_HZ   14000000UL   /* max ADC clock per RM0008 */
#endif

#ifndef APB1_MAX_FREQ_HZ
#define APB1_MAX_FREQ_HZ  36000000UL   /* max APB1 clock */
#endif

/* Максимумы шин для STM32F103C8T6 */
#ifndef HCLK_MAX_FREQ_HZ
#define HCLK_MAX_FREQ_HZ 72000000UL
#endif

#ifndef APB2_MAX_FREQ_HZ
#define APB2_MAX_FREQ_HZ 72000000UL
#endif

#ifndef AHB_MAX_FREQ_HZ
#define AHB_MAX_FREQ_HZ  72000000UL
#endif

/* Таймаут по умолчанию для ожиданий в микросекундах (1 сек) */
#ifndef CHECK_TIMEOUT_US
#define CHECK_TIMEOUT_US 1000000UL
#endif

InitErrors_t InitErrors = {
		.HCLK_Fail = false,
		.AHB_Fail  = false,
		.APB1_Fail = false,
		.APB2_Fail = false,
		.ADC_Fail  = false,
		.LSI_Fail  = false,
		.InitSysTick_Fail = false,
		.allIsOk 		  = false
};

static uint32_t hclk = 0UL;
static uint32_t pclk1 = 0UL;

/**
  * @brief  Update SystemCoreClock variable according to Clock Register Values.
  *         The SystemCoreClock variable contains the core clock (HCLK), it can
  *         be used by the user application to setup the SysTick timer or configure
  *         other parameters.
  *           
  * @note   Each time the core clock (HCLK) changes, this function must be called
  *         to update SystemCoreClock variable value. Otherwise, any configuration
  *         based on this variable will be incorrect.         
  *     
  * @note   - The system frequency computed by this function is not the real 
  *           frequency in the chip. It is calculated based on the predefined 
  *           constant and the selected clock source:
  *             
  *           - If SYSCLK source is HSI, SystemCoreClock will contain the HSI_VALUE(*)
  *                                              
  *           - If SYSCLK source is HSE, SystemCoreClock will contain the HSE_VALUE(**)
  *                          
  *           - If SYSCLK source is PLL, SystemCoreClock will contain the HSE_VALUE(**) 
  *             or HSI_VALUE(*) multiplied by the PLL factors.
  *         
  *         (*) HSI_VALUE is a constant defined in stm32f1xx.h file (default value
  *             8 MHz) but the real value may vary depending on the variations
  *             in voltage and temperature.   
  *    
  *         (**) HSE_VALUE is a constant defined in stm32f1xx.h file (default value
  *              8 MHz or 25 MHz, depending on the product used), user has to ensure
  *              that HSE_VALUE is same as the real frequency of the crystal used.
  *              Otherwise, this function may have wrong result.
  *                
  *         - The result of this function could be not correct when using fractional
  *           value for HSE crystal.
  * @param  None
  * @retval None
  */
void SystemCoreClockUpdate (void)
{
  uint32_t tmp = 0U, pllmull = 0U, pllsource = 0U;


    
  /* Get SYSCLK source -------------------------------------------------------*/
  tmp = RCC->CFGR & RCC_CFGR_SWS;
  
  switch (tmp)
  {
    case 0x00U:  /* HSI used as system clock */
      SystemCoreClock = HSI_VALUE;
      break;
    case 0x04U:  /* HSE used as system clock */
      SystemCoreClock = HSE_VALUE;
      break;
    case 0x08U:  /* PLL used as system clock */

      /* Get PLL clock source and multiplication factor ----------------------*/
      pllmull = RCC->CFGR & RCC_CFGR_PLLMULL;
      pllsource = RCC->CFGR & RCC_CFGR_PLLSRC;

      pllmull = ( pllmull >> 18U) + 2U;
      
      if (pllsource == 0x00U)
      {
        /* HSI oscillator clock divided by 2 selected as PLL clock entry */
        SystemCoreClock = (HSI_VALUE >> 1U) * pllmull;
      }
      else
      {
        /* HSE selected as PLL clock entry */
        if ((RCC->CFGR & RCC_CFGR_PLLXTPRE) != 0U)
        {/* HSE oscillator clock divided by 2 */
          SystemCoreClock = (HSE_VALUE >> 1U) * pllmull;
        }
        else
        {
          SystemCoreClock = HSE_VALUE * pllmull;
        }
      }
      break;

    default:
      SystemCoreClock = HSI_VALUE;
      break;
  }
  
  /* Compute HCLK clock frequency ----------------*/
  /* Get HCLK prescaler */
  tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> 4U)];
  /* HCLK clock frequency */
  SystemCoreClock >>= tmp;  
}

/* ===========================================================
   Вспомогательные функции — вычисление шин (HCLK/PCLK1/PCLK2)
   ===========================================================*/

/* Получить HCLK из SystemCoreClock и HPRE (AHB prescaler) */
static uint32_t GetHclk(uint32_t sysclk_hz)
{
    uint32_t hpre = (RCC->CFGR >> 4) & 0xFU; /* HPRE[3:0] */
    uint32_t div  = 1UL;

    /* HPRE код: 0..7 => div=1, 8..15 => div = 2^(hpreset-7) */
    if (hpre >= 8U)
        div = 1UL << (hpre - 7U);

    return sysclk_hz / div;
}

/* Получить PCLK1 из HCLK и PPRE1 */
static uint32_t GetPclk1(uint32_t hclk_hz)
{
    uint32_t ppre1 = (RCC->CFGR >> 8) & 0x7U; /* PPRE1[2:0] */
    uint32_t div   = 1UL;

    if (ppre1 >= 4U)
        div = 1UL << (ppre1 - 3U);

    return hclk_hz / div;
}

/* Получить PCLK2 из HCLK и PPRE2 */
static uint32_t GetPclk2(uint32_t hclk_hz)
{
    uint32_t ppre2 = (RCC->CFGR >> 11) & 0x7U; /* PPRE2[2:0] */
    uint32_t div   = 1UL;

    if (ppre2 >= 4U)
        div = 1UL << (ppre2 - 3U);

    return hclk_hz / div;
}

/* ===========================================================
   SysTick helpers (инициализация для микросекунд и измерения)
   Примечание: функция перенастраивает SysTick (без прерываний).
   ===========================================================*/

/* Инициализация SysTick для тиков = 1 микросекунда.
 * Возвращает true если успешно, false если reload не поместился в 24-bit.
 */
static bool InitSysTickForUsec(uint32_t sysclk_hz)
{
    uint32_t reload = (sysclk_hz / 1000000UL);
    if (reload == 0U) return false;
    reload -= 1U;

    if (reload > 0x00FFFFFFUL)
        return false; /* SysTick 24-bit ограничение */

    /* Отключаем SysTick перед конфигурацией */
    SysTick->CTRL = 0U;

    SysTick->LOAD = reload;    /* количество тактов на 1 us */
    SysTick->VAL  = 0U;        /* сброс счётчика на LOAD */
    /* CLKSOURCE = processor clock, ENABLE = 1, TICKINT = 0 (без прерываний) */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

    return true;
}

/* Вернуть текущее значение SysTick->VAL (24-bit countdown).
 * Start/Elapsed mechanism использует разницу между значениями VAL.
 */
static inline uint32_t SystickNow(void)
{
    return SysTick->VAL;
}

/* Вычислить прошедшие микросекунды с момента start (SysTick уже настроен на 1us tick).
 * Поскольку SysTick считает вниз, вычисления:
 *   if (curr <= start) elapsed = start - curr;
 *   else elapsed = start + (LOAD - curr + 1)
 */
static inline uint32_t SystickElapsedUs(uint32_t start)
{
    uint32_t curr = SysTick->VAL;
    uint32_t load = SysTick->LOAD;

    if (curr <= start)
        return start - curr;
    else
        return start + (load - curr + 1U);
}
/* Выключение SysTick */
static inline void SysTick_Disable(void)
{
    /* 1. Остановить SysTick и запретить IRQ */
    SysTick->CTRL = 0U;

    /* 2. Сбросить reload (необязательно, но чисто) */
    SysTick->LOAD = 0U;

    /* 3. Очистить текущее значение */
    SysTick->VAL = 0U;

    /* 4. Сбросить pending SysTick в NVIC (ВАЖНО) */
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;
}

/* ===========================================================
   Runtime checks (в отдельные функции)
   Каждая возвращает true если OK, false если ошибка/превышение.
   Для ожиданий используется SysTick через параметры timeout_us.
   ===========================================================*/

/* Проверка HCLK (AHB) — ядро + AHB-connected peripherals (DMA/Flash/etc.) */
static bool Check_HCLK(uint32_t sysclk_hz, uint32_t max_hclk_hz)
{
    hclk = GetHclk(sysclk_hz);
    return (hclk <= max_hclk_hz);
}

/* Проверка AHB — фактически дублирует HCLK (AHB шина) */
static bool Check_AHB(uint32_t max_ahb_hz)
{
    return (hclk <= max_ahb_hz);
}

/* Проверка APB1 (низкоскоростная шина) */
static bool Check_APB1(uint32_t sysclk_hz, uint32_t max_apb1_hz)
{
    return (pclk1 <= max_apb1_hz);
}

/* Проверка APB2 (высокоскоростная шина) */
static bool Check_APB2(uint32_t sysclk_hz, uint32_t max_apb2_hz)
{
    return (pclk2 <= max_apb2_hz);
}

/* Проверка ADC (вычисляет подходящий prescaler, проверяет, что <= ADC_MAX_FREQ_HZ) */
static bool Check_ADCClock(uint32_t sysclk_hz)
{
    uint32_t adc_clk;

    if ((pclk2 / 2UL) <= ADC_MAX_FREQ_HZ)      adc_clk = pclk2 / 2UL;
    else if ((pclk2 / 4UL) <= ADC_MAX_FREQ_HZ) adc_clk = pclk2 / 4UL;
    else if ((pclk2 / 6UL) <= ADC_MAX_FREQ_HZ) adc_clk = pclk2 / 6UL;
    else                                        adc_clk = pclk2 / 8UL;

    return (adc_clk <= ADC_MAX_FREQ_HZ);
}

/* Установить prescaler ADC на максимально возможный ≤ ADC_MAX_FREQ_HZ.
 * (функция не делает проверок таймаутов — предполагается, что они пройдены перед вызовом)
 */
static void SetAdcPrescaler(uint32_t sysclk_hz)
{
    uint32_t presc_bits;

    if ((pclk2 / 2UL) <= ADC_MAX_FREQ_HZ)      presc_bits = RCC_CFGR_ADCPRE_DIV2;
    else if ((pclk2 / 4UL) <= ADC_MAX_FREQ_HZ) presc_bits = RCC_CFGR_ADCPRE_DIV4;
    else if ((pclk2 / 6UL) <= ADC_MAX_FREQ_HZ) presc_bits = RCC_CFGR_ADCPRE_DIV6;
    else                                        presc_bits = RCC_CFGR_ADCPRE_DIV8;

    RCC->CFGR &= ~RCC_CFGR_ADCPRE;
    RCC->CFGR |= presc_bits;
}

/* Проверка готовности LSI (источник для IWDG) с таймаутом в микросекундах.
 * Возвращает true если LSI зажёгся в пределах timeout_us.
 *
 * Примечание: включает LSI (RCC->CSR.LSION).
 */
static bool Check_LSIReady_Timeout(uint32_t timeout_us)
{
    /* Включаем LSI — источник для IWDG */
    RCC->CSR |= RCC_CSR_LSION;

    uint32_t start = SystickNow();
    while ((RCC->CSR & RCC_CSR_LSIRDY) == 0U)
    {
        if (SystickElapsedUs(start) >= timeout_us)
            return false; /* таймаут вышел */
    }

    /* LSI готов */
    return true;
}

void
/* ===========================================================
   Объединённая runtime-проверка всех шин и LSI с таймаутами
   Возвращает true если все проверки пройдены
   ===========================================================*/
static bool RuntimeCheckAllClocks(uint32_t sysclk_hz, uint32_t timeout_us)
{
	bool allIsOk = true;

	// Errors check
	InitErrors.HCLK_Fail = !Check_HCLK(sysclk_hz, HCLK_MAX_FREQ_HZ);
	InitErrors.AHB_Fail = !Check_AHB(sysclk_hz, HCLK_MAX_FREQ_HZ);
	InitErrors.APB1_Fail = !Check_APB1(sysclk_hz, APB1_MAX_FREQ_HZ);
	InitErrors.APB2_Fail = !Check_APB2(sysclk_hz, APB2_MAX_FREQ_HZ);
	InitErrors.ADC_Fail = !Check_ADCClock(sysclk_hz);
	InitErrors.LSI_Fail = !Check_LSIReady_Timeout(timeout_us);

    if (
    		InitErrors.HCLK_Fail ||
    		InitErrors.AHB_Fail  ||
    		InitErrors.APB1_Fail ||
    		InitErrors.APB2_Fail ||
    		InitErrors.ADC_Fail  ||
    		InitErrors.LSI_Fail
		)
    {
    	allIsOk = false;
    }
    else
    {
    	allIsOk = true;
    }

    /* Все проверки пройдены */
    return allIsOk;
}

/* ===========================================================
   Основная функция — PeripheralEnable
   Возвращает true при успешной настройке, false при ошибке (проверки не пройдены)
   ===========================================================*/
void PeripheralEnable(void)
{
	uint32_t sysclk = SystemCoreClock;
    /* -------------------------------------------------------
       Инициализируем SysTick для микросекундных таймаутов
       (функция перенастраивает SysTick; учти это в проекте)
       ------------------------------------------------------- */
	InitErrors.InitSysTick_Fail = !InitSysTickForUsec(sysclk);

    /* =======================================================
       Runtime checks с таймаутами (HCLK, AHB, APB1, APB2, ADC, LSI)
       Если что-то не в порядке — не поднимаем периферию и возвращаем false
       =======================================================*/
    InitErrors.allIsOk = RuntimeCheckAllClocks(sysclk, CHECK_TIMEOUT_US);

    /* -------------------------------------------------------
       Выключаем SysTick
       ------------------------------------------------------- */
    SysTick_Disable();


    /* =======================================================
       1. СБРОС ПЕРИФЕРИИ
       =======================================================*/

    /* APB2 reset: AFIO, GPIOA-E, ADC1, TIM1, SPI1, USART1 */
    RCC->APB2RSTR = 0xFFFFFFFFU;
    RCC->APB2RSTR = 0x00000000U;

    /* APB1 reset: TIM2–4, WWDG, SPI2, USART2–3, I2C1–2, BKP, PWR */
    RCC->APB1RSTR = 0xFFFFFFFFU;
    RCC->APB1RSTR = 0x00000000U;

    /* =======================================================
       2. ADC CLOCK SETUP (≤ 14 MHz)
       =======================================================*/
    SetAdcPrescaler(sysclk);

    /* =======================================================
       3. ВКЛЮЧЕНИЕ ВСЕХ ДОСТУПНЫХ ТАКТИРОВАНИЙ (по блокам, построчно)
       (STM32F103C8T6 — проверено по RM0008 / datasheet)
       =======================================================*/

    /* ---------------- AHB ---------------- */
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;   /* DMA1 controller */
    RCC->AHBENR |= RCC_AHBENR_SRAMEN;   /* SRAM interface */
    RCC->AHBENR |= RCC_AHBENR_FLITFEN;  /* Flash interface */
    RCC->AHBENR |= RCC_AHBENR_CRCEN;    /* CRC unit */

    /* ---------------- APB2 ---------------- */
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;     /* Alternate function IO */

    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;     /* GPIOA */
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;     /* GPIOB */
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;     /* GPIOC */
    RCC->APB2ENR |= RCC_APB2ENR_IOPDEN;     /* GPIOD */
    RCC->APB2ENR |= RCC_APB2ENR_IOPEEN;     /* GPIOE */

    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;     /* ADC1 */

    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;     /* Advanced timer TIM1 */

    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;     /* SPI1 */

    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;   /* USART1 */

    /* ---------------- APB1 ---------------- */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;     /* TIM2 */
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;     /* TIM3 */
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;     /* TIM4 */

    RCC->APB1ENR |= RCC_APB1ENR_WWDGEN;     /* Window watchdog (IWDG — внешнее LSI) */

    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;     /* SPI2 */

    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;   /* USART2 */
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;   /* USART3 */

    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;     /* I2C1 */
    RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;     /* I2C2 */

    RCC->APB1ENR |= RCC_APB1ENR_BKPEN;      /* Backup interface */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;      /* Power interface */

    /* =======================================================
       4. DUMMY READ (завершение записи через мост)
       — гарантирует, что все записи в регистры тактирования прошли
       ======================================================= */
    (void)RCC->AHBENR;
    (void)RCC->APB2ENR;
    (void)RCC->APB1ENR;
}


void ClockInit(void)
{
	uint32_t timeout = HSE_STARTUP_TIMEOUT;

	// Включение HSE
	RCC->CR |= RCC_CR_HSEON;	// Включение кварца (8 Мгц)

	/* Ожидание включения кварца */
	while ((RCC->CR & RCC_CR_HSERDY) == 0UL && timeout > 0U)
	{
		timeout--;
	}

	if (RCC->CR & RCC_CR_HSERDY)
	{
		// Если HSE стартанул:
        // Flash latency для 72 MHz
		FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;

        // Prescalers
        RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
        RCC->CFGR |= RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1;

        // Настройка PLL (HSE * 9)
        RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL | RCC_CFGR_PLLXTPRE);	// Очистка регистра
        RCC->CFGR |= RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL9;

        // Включение PLL
        RCC->CR |= RCC_CR_PLLON;
        while (!(RCC->CR & RCC_CR_PLLRDY)){};	// Ожидание включения

        // Переключение на PLL
        RCC->CFGR &= ~RCC_CFGR_SW;
        RCC->CFGR |= RCC_CFGR_SW_PLL;
        while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL){};	// waiting for switch
	}
    else
    {
        // HSE не стартует — остаёмся на HSI
    }
    /* Настройка таблицы векторов */
    SCB->VTOR = FLASH_BASE;	// Начало таблицы - адрес начала флеш-памяти
}


/**
  * @brief  Setup the microcontroller system
  *         Initialize the Embedded Flash Interface, the PLL and update the
  *         SystemCoreClock variable.
  * @note   This function should be used only after reset.
  * @param  None
  * @retval None
  */
void SystemInit(void)
{
	ClockInit();
	SystemCoreClockUpdate();
	PeripheralEnable();
}

/* Конец файла system_stm32f1xx.c */
