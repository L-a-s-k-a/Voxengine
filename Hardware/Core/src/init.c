#include "init.h"
#include "app_data.h"

/**
 * @brief Тактирование ядра на 216 МГц.
 *
 * Источник: HSE в режиме BYPASS (на Nucleo-144 от ST-Link приходит
 * готовый меандр 8 МГц на OSC_IN, а не сигнал с кварца).
 * Схема: HSE(8) / M(8) = 1 МГц -> * N(432) = 432 МГц -> / P(2) = 216 МГц.
 *       -> / Q(9) = 48 МГц (для USB, сейчас не используется).
 *
 * Почему over-drive: для F7 на 216 МГц обязателен режим Over-drive,
 * иначе ядро не запустится на полной частоте.
 * Почему 7WS: 216 МГц при 3.3 В требуют 7 wait states для Flash.
 */
void SystemClock_Config(void)
{
    // 1. Включаем режим BYPASS (указывает, что на вход OSC_IN подается уже готовый меандр)
    RCC->CR |= RCC_CR_HSEBYP;
    // Включаем HSE (внешний кварц)
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY))
        ;

    // 2. Включаем питание модуля перегрузки частоты (Over-drive) для достижения 216 МГц
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR1 |= PWR_CR1_ODEN;
    while (!(PWR->CSR1 & PWR_CSR1_ODRDY))
        ;

    PWR->CR1 |= PWR_CR1_ODSWEN;
    while (!(PWR->CSR1 & PWR_CSR1_ODSWRDY))
        ;

    // 3. Настройка задержки Flash (7 Wait States необходимы для работы на 216 МГц при 3.3V)
    FLASH->ACR = FLASH_ACR_LATENCY_7WS | FLASH_ACR_PRFTEN;

    // 4. Конфигурация главного PLL: HSE = 8MHz. Цель = 216MHz.
    // Формула: Частота = (HSE / M) * N / P
    // M = 8 (8/8 = 1MHz на входе PLL), N = 432 (1*432 = 432MHz), P = 2 (432/2 = 216MHz)
    // Q = 9 (для шины USB 48MHz, если понадобится: 432/9 = 48MHz)
    RCC->PLLCFGR = (8 << RCC_PLLCFGR_PLLM_Pos) |
                   (432 << RCC_PLLCFGR_PLLN_Pos) |
                   (0 << RCC_PLLCFGR_PLLP_Pos) | // 00 означает P=2
                   (RCC_PLLCFGR_PLLSRC_HSE) |
                   (9 << RCC_PLLCFGR_PLLQ_Pos);

    // Включаем PLL
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY))
        ;

    // 5. Настройка делителей шин: AHB = 1 (216MHz), APB1 = 4 (54MHz), APB2 = 2 (108MHz)
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1 |
                 RCC_CFGR_PPRE1_DIV4 |
                 RCC_CFGR_PPRE2_DIV2;

    // Переключаем системное тактирование на PLL
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL)
        ;
}

/**
 * @brief SysTick на 1 кГц — "сердце" всей временной логики.
 *
 * В SysTick_Handler обрабатываются:
 *  - дебаунс/удержание кнопки PC13 (FSM);
 *  - отсчёт времени отсутствия пассажиров PIR.
 *
 * LOAD = 216000 - 1 = 215999, источник — AHB без деления.
 */
void SysTick_Init_Config(void)
{
    CLEAR_BIT(SysTick->CTRL, SysTick_CTRL_ENABLE_Msk);  // На всякий случай, предварительно, выключим счётчик
    SET_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);   // Разрешаем прерывание по системному таймеру
    SET_BIT(SysTick->CTRL, SysTick_CTRL_CLKSOURCE_Msk); // Источник тактирования будет идти из AHB без деления
    MODIFY_REG(SysTick->LOAD, SysTick_LOAD_RELOAD_Msk,
               215999 << SysTick_LOAD_RELOAD_Pos);      // Значение с которого начинается счёт, эквивалентное 1 кГц
    CLEAR_REG(SysTick->VAL);     // Очистка поля
    SET_BIT(SysTick->CTRL, SysTick_CTRL_ENABLE_Msk);    // Включим счётчик

    NVIC_SetPriority(SysTick_IRQn, 4);
}

/**
 * @brief Настройка GPIO:
 *  PA0, PA3  — analog (входы АЦП).
 *  PA4       — вход с pull-down (кнопка SETUP).
 *  PB0, PB7, PB14 — выходы (PB14 — PWR_LED; PB0/PB7 не используются).
 *  PC8..PC10 — выходы LED режимов.
 *  PE9       — AF1 = TIM1_CH1 (ШИМ на двигатель).
 *
 * Все LED сразу гасим (BSRR -> BRx), чтобы после reset не горели.
 * FIXME: PC13 (кнопка) и PB3 (PIR) не настроены явно — полагаемся
 * на reset-состояние и внешние цепи.
 */
void GPIO_Config(void){
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN;

    /* PA0, PA3 — аналоговый режим (для ADC1_IN0 / ADC1_IN3). */
    GPIOA->MODER |= GPIO_MODER_MODER0 | GPIO_MODER_MODER3; 
    /* PA4 — pull-down (кнопка SETUP подтягивает к VCC при нажатии). */
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR4;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR4_1;

    /* PB14, PB7, PB0 — general purpose output. */
    GPIOB->MODER |= GPIO_MODER_MODER14_0 | GPIO_MODER_MODER7_0 | GPIO_MODER_MODER0_0;
    GPIOB->BSRR  |= GPIO_BSRR_BR14 | GPIO_BSRR_BR7 | GPIO_BSRR_BR0;

    /* PC8..PC10 — выходы на LED. */
    GPIOC->MODER &= ~GPIO_MODER_MODER13;
    GPIOC->MODER |= GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0 | GPIO_MODER_MODER10_0;
    GPIOC->BSRR  |= GPIO_BSRR_BR8 | GPIO_BSRR_BR9 | GPIO_BSRR_BR10;

    /* PE9 -> AF1 (TIM1_CH1): MODER = 10, AFR[1].AFRH1 = 0001. */
    GPIOE->MODER  &= ~GPIO_MODER_MODER9_Msk; 
    GPIOE->MODER  |= (2 << GPIO_MODER_MODER9_Pos); // PE9 -> alternative function
    GPIOE->AFR[1] &= ~GPIO_AFRH_AFRH1_Msk;
    GPIOE->AFR[1] |= GPIO_AFRH_AFRH1_0; // PE9 -> CCR1 from TIM1 (PWM)
}

/**
 * @brief Внешние прерывания.
 *
 *  EXTI3  (PB3)  — PIR-датчик. Прерывание по любому изменению уровня
 *                  (сейчас включены RTSR и FTSR), в обработчике
 *                  читаем IDR и ставим pir_sens.
 *  EXTI4  (PA4)  — кнопка SETUP.
 *  EXTI13 (PC13) — пользовательская кнопка B1 (Nucleo).
 *
 * Приоритеты: EXTI3 (PIR) < EXTI4 (setup) < EXTI15_10 (кнопка)
 * по числу = выше приоритет (меньше число). То есть PIR самый срочный.
 */
void EXTI_Config(void){
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI13_PC; // линия 13 <- порт C
    SYSCFG->EXTICR[1] |= SYSCFG_EXTICR2_EXTI4_PA;  // линия 4  <- порт A
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI3_PB;  // линия 3  <- порт B

     /* Маска: разрешаем прерывания от линий 3, 4, 13. */
    EXTI->IMR |= EXTI_IMR_MR3 | EXTI_IMR_MR4 | EXTI_IMR_MR13;

    /* Фронты: */
    EXTI->RTSR |= EXTI_RTSR_TR13 | EXTI_RTSR_TR3 | EXTI_RTSR_TR4;
    EXTI->FTSR |= EXTI_FTSR_TR3; // PIR по любому фронту

    NVIC_SetPriority(EXTI3_IRQn, 1);
    NVIC_EnableIRQ(EXTI3_IRQn);
    NVIC_SetPriority(EXTI4_IRQn, 2);
    NVIC_EnableIRQ(EXTI4_IRQn);
    NVIC_SetPriority(EXTI15_10_IRQn, 3);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/**
 * @brief TIM1 в режиме PWM mode 1 на канале 1.
 *
 * PSC/ARR берутся из init_config (в main: PSC=216, ARR=1000).
 * При тактировании TIM1 от APB2=108 МГц:
 *   f_PWM = 108e6 / (PSC * ARR) = 108e6 / (216*1000) = 500 Гц.
 * 500 Гц — типично для коллекторного двигателя: не пищит, но и не
 * перегружает моторный драйвер.
 *
 * CCR1 (скважность) меняется из main: g_app.pulse -> TIM1->CCR1.
 * TRGO = Update event — этим сигналом триггерится ADC.
 */
void TIM1_Config(void){
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    TIM1->PSC  = init_config.PSC - 1;
    TIM1->ARR  = init_config.ARR - 1;

    /* Канал 1: PWM mode 1, preload enable (CCR обновляется на UEV,
     * чтобы не было глитчей при изменении скважности). 
     */
    TIM1->CCMR1 &= ~TIM_CCMR1_CC1S_Msk;         // CC1 = output
    TIM1->CCMR1 |= TIM_CCMR1_OC1PE;             // Preload
    TIM1->CCMR1 &= ~TIM_CCMR1_OC1M;
    TIM1->CCMR1 |= (6 << TIM_CCMR1_OC1M_Pos);   // 110 - PWM mode 1
    TIM1->CCER  |= TIM_CCER_CC1E;               // Включаем выход канала 1
    TIM1->BDTR  |= TIM_BDTR_MOE;                // MOE обязателен для выходов

    TIM1->EGR   |= TIM_EGR_UG;                  // Применить PSC/ARR сейчас
    TIM1->CR2   |= (2 << TIM_CR2_MMS_Pos);      // TRGO = Update event (для ADC)
    TIM1->CR1   |= TIM_CR1_CEN;                 // Запуск счёта
}

/**
 * @brief ADC1 + DMA2_Stream0.
 *
 * Идея: TIM1 генерирует TRGO на каждом Update event (500 Гц).
 * По этому триггеру ADC1 сканирует два канала (0 и 3) и через DMA
 * складывает результаты в g_app.adc_buffer[0..1]. CPU не участвует.
 * В main из этого буфера делает mean_filter() и transform().
 *
 * Настройки ADC:
 *  - 12 бит, right-aligned;
 *  - scan mode (два канала в SQR);
 *  - EXTSEL = TIM1_TRGO, rising.
 *
 * DMA: циклический режим, периферия -> память, 16 бит/16 бит, MINC.
 */
void ADC1_DMA_Config(void){
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;         // DMA2

    /* Настройка DMA2 Stream0 для ADC1 */
    DMA2_Stream0->CR &= ~DMA_SxCR_EN;           // Выключаем поток перед настройкой
    while (DMA2_Stream0->CR & DMA_SxCR_EN) {}   // Ждём фактического выключения

    DMA2_Stream0->CR = 0;                       // Сброс регистра
    // Канал 0 (CHSEL = 0), направление периферия->память (DIR = 0),
    // размер данных 16 бит (MSIZE = 01, PSIZE = 01),
    // инкремент памяти (MINC = 1), циклический режим (CIRC = 1)
    DMA2_Stream0->CR |= (0U << DMA_SxCR_CHSEL_Pos)   // Channel 0 для ADC1
                     |  (0U << DMA_SxCR_DIR_Pos)     // Peripheral-to-memory
                     |  (1U << DMA_SxCR_MSIZE_Pos)   // 16-bit memory
                     |  (1U << DMA_SxCR_PSIZE_Pos)   // 16-bit peripheral
                     |  (1U << DMA_SxCR_MINC_Pos)    // Memory increment
                     |  (1U << DMA_SxCR_CIRC_Pos);   // Circular mode

    DMA2_Stream0->PAR  = (uint32_t)&(ADC1->DR);                             // Адрес регистра данных ADC
    DMA2_Stream0->M0AR = (uint32_t)(uintptr_t)(void *)&g_app.adc_buffer[0]; // Адрес буфера в памяти
    DMA2_Stream0->NDTR = ADC_BUF_LEN;                                       // Количество передач

    /* Сброс флагов потока перед запуском. */
    DMA2->LIFCR = DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTEIF0 | DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CFEIF0;
    DMA2_Stream0->CR |= DMA_SxCR_EN;                 // Включаем поток

    /* --- Тактирование ADC --- */
    /* ADCPRE = 4: 108 / 4 = 27 МГц. Максимум для ADC на F7 — 36 МГц. */
    ADC->CCR |= ADC_CCR_ADCPRE_0; 
    
    /* --- ADC1 --- */
    ADC1->CR1 = 0;
    ADC1->CR1 &= ~ADC_CR1_RES;              // Resolution 12 bit (RES = 00)
    ADC1->CR1 |= ADC_CR1_SCAN;              // Сканируем несколько каналов

    ADC1->CR2 = 0;
    ADC1->CR2 &= ~ADC_CR2_ALIGN;            // Data right alignment
    ADC1->CR2 &= ~ADC_CR2_CONT;             // Отключение непрерывного сканирования
    ADC1->CR2 |= (9 << ADC_CR2_EXTSEL_Pos); // Начинать преобразования по внешнему триггеру от таймера
    ADC1->CR2 |= ADC_CR2_EXTEN_0;           // Rising trigger on
    ADC1->CR2 |= ADC_CR2_DMA;               // DMA mode enabled
    ADC1->CR2 |= ADC_CR2_DDS;               // DMA-запросы не снимаются между преобразованиями 

    /* Время выборки 56 тактов (SMP=011) — компромисс точность/скорость. */
    ADC1->SMPR2 |= (3 << ADC_SMPR2_SMP0_Pos); 
    ADC1->SMPR2 |= (3 << ADC_SMPR2_SMP3_Pos);

    /* Две конверсии: SQ1=ch0, SQ2=ch3. */
    ADC1->SQR1 = 0;
    ADC1->SQR1 |= ADC_SQR1_L_0;            // L = 1 -> 2 conversion
    ADC1->SQR3 = 0;
    ADC1->SQR3 |= (0 << ADC_SQR3_SQ1_Pos); // SQ1 -> channel 0
    ADC1->SQR3 |= (3 << ADC_SQR3_SQ2_Pos); // SQ2 -> channel 3

    ADC1->CR2 |= ADC_CR2_ADON;                  // ADC on without conversion
    for (volatile int i = 0; i < 1000; i++) {}  // tSTAB — пауза после ADON
}

