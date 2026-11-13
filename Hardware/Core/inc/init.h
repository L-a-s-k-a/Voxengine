#ifndef INIT_H
#define INIT_H

/* CMSIS-заголовки. stm32f756xx.h тянет за собой stm32f7xx.h,
 * поэтому формально одного достаточно; оставил оба — как есть. 
 */
#include "stm32f7xx.h"
#include "stm32f756xx.h"

/* Инициализация — вызывается по порядку из main().
 * Порядок важен: SystemClock_Config должен быть до всего,
 * что зависит от частоты (SysTick, TIM, ADC). 
 */
void SystemClock_Config(void);  // 216 МГц через PLL от HSE (MCO ST-Link)
void SysTick_Init_Config(void); // 1 кГц, источник — AHB
void GPIO_Config(void);         // LED, входы кнопок, AF для ШИМ
void EXTI_Config(void);         // EXTI3 (PIR), EXTI4 (setup), EXTI13 (btn)
void TIM1_Config(void);         // ШИМ на PE9 (TIM1_CH1)
void ADC1_DMA_Config(void);     // ADC1 по триггеру TIM1_TRGO + DMA

#endif