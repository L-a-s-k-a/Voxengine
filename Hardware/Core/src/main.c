#include "init.h"
#include "app_data.h"

/**
 * @brief Точка входа.
 */
int main(void)
{
    init_config.PSC = 216;
    init_config.ARR = 1000;
    SystemClock_Config();
    SysTick_Init_Config();
    GPIO_Config();
    EXTI_Config();
    TIM1_Config();
    ADC1_DMA_Config();

    while (1)
    {

    }
}