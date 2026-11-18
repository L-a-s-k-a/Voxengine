#include "rcc.h"

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