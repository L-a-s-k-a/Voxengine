#include "main.h"

void P10_TestPixel(void);

/**
 * @brief Точка входа.
 */
int main(void)
{
    init_config.PSC = 216;
    init_config.ARR = 1000;
    SystemClock_Config();
    
    GPIO_Config();
    SPI1_Init();

    P10_TestPixel();
    // HUB12_SetPixel(1, 1, true); // NADA CHECKNUT!!!

    while (1)
    {

    }
}

/*
CLK	        PA5 (SPI1 SCK)
R	        PA7 (SPI1 MOSI)
SCLK(LAT)   PG10 Legasy PB0
OE	        PG11 Legasy PB1
A	        PG12 Legasy PB2
B	        PG13 Legasy PB10
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
        0xFF, 0xBF, 0xFF, 0xFF
    };

    /* Disable display during data transfer */

    GPIOG->BSRR = (1U << (11U + 16U));

    /* Select scan address 00 */

    GPIOG->BSRR = (1U << (12U + 16U));
    GPIOG->BSRR = (1U << (13U + 16U));

    /* Send 128 bits */

    for (uint32_t i = 0; i < 16U; i++)
    {
        SPI1_TransmitByte(data[i]);
    }

    /* Latch data: rising edge */

    GPIOG->BSRR = (1U << (10U + 16U));
    GPIOG->BSRR = (1U << 10U);

    /* Enable display */

    GPIOG->BSRR = (1U << 11U);
}