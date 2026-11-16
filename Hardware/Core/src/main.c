#include "init.h"
#include "app_data.h"
#include "spi.h"

void P10_TestPixel(void);

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
    // ADC1_DMA_Config();
    SPI1_Init();

    P10_TestPixel();

    while (1)
    {
        // if(g_app.global_tick_count > 1000){
        //     SPI1_TransmitByte(240);
        //     g_app.global_tick_count = 0;
        // } 
    }
}

/*
CLK	        PA5 (SPI1 SCK)
R	        PB5 (SPI1 MOSI)
SCLK(LAT)   PB0
OE	        PB1
A	        PB2
B	        PB10
*/
void P10_TestPixel(void)
{
    /*
     * 128 bits = 16 bytes.
     * 1 pixel ON, all other pixels OFF.
     *
     * Output LOW  -> LED ON
     * Output HIGH -> LED OFF
     */

    const uint8_t data[16] =
    {
        0xFF, 0xFF, 0xFF, 0x7F,
        0xFF, 0xFF, 0xBF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF
    };

    /* Disable display during data transfer */

    GPIOB->BSRR = (1U << (1U + 16U));

    /* Select scan address 00 */

    GPIOB->BSRR = (1U << (2U + 16U));
    GPIOB->BSRR = (1U << (10U + 16U));

    /* Send 128 bits */

    for (uint32_t i = 0; i < 16U; i++)
    {
        SPI1_TransmitByte(data[i]);
    }

    /* Latch data: rising edge */

    GPIOB->BSRR = (1U << (0U + 16U));
    GPIOB->BSRR = (1U << 0U);

    /* Enable display */

    GPIOB->BSRR = (1U << 1U);
}