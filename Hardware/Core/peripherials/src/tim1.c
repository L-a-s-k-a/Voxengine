#include "tim1.h"
#include "app_data.h"

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
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;

    /* PE9 -> AF1 (TIM1_CH1): MODER = 10, AFR[1].AFRH1 = 0001. */
    GPIOE->MODER  &= ~GPIO_MODER_MODER9_Msk; 
    GPIOE->MODER  |= (2 << GPIO_MODER_MODER9_Pos); // PE9 -> alternative function
    GPIOE->AFR[1] &= ~GPIO_AFRH_AFRH1_Msk;
    GPIOE->AFR[1] |= GPIO_AFRH_AFRH1_0; // PE9 -> CCR1 from TIM1 (PWM)

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
