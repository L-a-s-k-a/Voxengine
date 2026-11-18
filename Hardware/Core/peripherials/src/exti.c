#include "exti.h"

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
