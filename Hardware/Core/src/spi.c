#include "spi.h"
#include "app_data.h"

#define DMA_SPI_BUFFER_SIZE 2

/*
 * SPI1:
 * PA5 -> SCK
 * PB5 -> MOSI
 *
 * SPI Mode 0
 * Master
 * 8-bit
 * MSB First
 * 1-line TX mode
 */

void SPI1_Init(void)
{
    /* Enable GPIOA clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* 2. Configure PA5 and PA7 as Alternate Function */
    GPIOA->MODER &= ~(GPIO_MODER_MODER5_Msk | GPIO_MODER_MODER7_Msk);
    GPIOA->MODER |= (2U << GPIO_MODER_MODER5_Pos) |
                    (2U << GPIO_MODER_MODER7_Pos);
    GPIOA->AFR[0] &= ~(GPIO_AFRL_AFRL5_Msk | GPIO_AFRL_AFRL7_Msk);
    GPIOA->AFR[0] |= (5U << GPIO_AFRL_AFRL5_Pos) |
                     (5U << GPIO_AFRL_AFRL7_Pos);
    GPIOB->MODER &= ~GPIO_MODER_MODER5;
    GPIOB->MODER |= (2U << GPIO_MODER_MODER5_Pos);
    GPIOB->AFR[0] &= ~GPIO_AFRL_AFRL5_Msk;
    GPIOB->AFR[0] |= (5U << GPIO_AFRL_AFRL5_Pos);

    /* Enable SPI1 peripheral clock */
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    DMA2_Stream3->CR &= ~DMA_SxCR_EN; // Выключить поток для настройки
    DMA2_Stream3->PAR = (uint32_t)&(SPI1->DR); // Периферийный адрес (регистр данных)
    DMA2_Stream3->M0AR = (uint32_t)g_app.frame_buffer; // Адрес буфера в памяти
    DMA2_Stream3->NDTR = DMA_SPI_BUFFER_SIZE; // Количество байт для передачи
    DMA2_Stream3->CR = DMA_SxCR_CHSEL_0 | DMA_SxCR_CHSEL_1 | // Channel 3 (пример)
                        DMA_SxCR_MSIZE_0 | // Memory size 8-bit
                        DMA_SxCR_PSIZE_0 | // Peripheral size 8-bit
                        DMA_SxCR_MINC |    // Memory increment mode
                        DMA_SxCR_DIR_0;    // Memory-to-Peripheral
    DMA2_Stream3->CR |= DMA_SxCR_EN; // Включить поток

    /* 8. Disable SPI before configuration */
    SPI1->CR1 = 0U;
    SPI1->CR2 = 0U;

    /*
     * 9. Configure SPI1
     *
     * MSTR     = 1  Master
     * BR       = 110 / 128 (fPCLK/128 = 108/128 = 843.75 kHz)
     * CPOL     = 0
     * CPHA     = 0
     * LSBFIRST = 0
     * SSM      = 1
     * SSI      = 1
     * BIDIMODE = 1
     * BIDIOE   = 1
     */

    SPI1->CR1 =
        SPI_CR1_MSTR |
        (6U << SPI_CR1_BR_Pos) | // BR = 110 / 128
        SPI_CR1_SSM |
        SPI_CR1_SSI |
        SPI_CR1_BIDIMODE |
        SPI_CR1_BIDIOE;

    SPI1->CR2 |= SPI_CR2_TXDMAEN; // Enable TX DMA

    /* 10. Enable SPI1 */
    SPI1->CR1 |= SPI_CR1_SPE;
}

void SPI1_TransmitByte(uint8_t data)
{
    /* Wait until transmit buffer is empty */

    while (!(SPI1->SR & SPI_SR_TXE))
    {
    }

    /* Write data */

    *(__IO uint8_t *)&SPI1->DR = data;

    /* Wait until transmission is complete */

    while (SPI1->SR & SPI_SR_BSY)
    {
    }
}

void SPI1_TransmitBuffer(
    const uint8_t *data,
    uint32_t length
)
{
    if ((data == 0) || (length == 0U))
    {
        return;
    }

    while (length--)
    {
        SPI1_TransmitByte(*data++);
    }
}