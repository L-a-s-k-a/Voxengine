#include "init.h"
#include "app_data.h"
#include "spi.h"

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
    SPI1_Init();

    while (1)
    {
        if(g_app.global_tick_count > 1000){
            SPI1_TransmitByte(240);
            g_app.global_tick_count = 0;
        } 
    }
}