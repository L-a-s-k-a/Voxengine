#include "spi.h"
#include "app_data.h"

/*
 * DMA buffer.
 *
 * Place this section in DMA-accessible SRAM1.
 * Do not place it in DTCM.
 */
__attribute__((section(".dma_buffer")))
__attribute__((aligned(32)))
static volatile uint8_t dma_buffer[LED_ROW_BYTES];

void SPI1_Init(void){
    SPI_GPIO_Config();
    SPI1_Config();
    SPI_DMA_Init();
}

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
void SPI1_Config(void)
{
    /* Enable SPI1 peripheral clock */
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* 8. Disable SPI before configuration */
    SPI1->CR1 = 0U;
    SPI1->CR2 = 0U;

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

static void SPI_DMA_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;

    LED_DMA_STREAM->CR &= ~DMA_SxCR_EN;

    while (LED_DMA_STREAM->CR & DMA_SxCR_EN)
    {
        /* Initialization only */
    }

    DMA2->LIFCR =
        DMA_LIFCR_CTCIF3 |
        DMA_LIFCR_CHTIF3 |
        DMA_LIFCR_CTEIF3 |
        DMA_LIFCR_CDMEIF3 |
        DMA_LIFCR_CFEIF3;

    LED_DMA_STREAM->PAR = (uint32_t)&SPI1->DR;

    LED_DMA_STREAM->M0AR = (uint32_t)dma_buffer;

    LED_DMA_STREAM->NDTR = LED_ROW_BYTES;

    LED_DMA_STREAM->CR =
        (3U << DMA_SxCR_CHSEL_Pos) |
        DMA_SxCR_DIR_0 |
        DMA_SxCR_MINC |
        DMA_SxCR_TCIE |
        DMA_SxCR_TEIE |
        DMA_SxCR_PL_1;

    /*
     * Peripheral width = 8 bit
     * Memory width = 8 bit
     * Memory increment enabled
     * Normal mode
     * Memory-to-peripheral
     */

    NVIC_SetPriority(DMA2_Stream3_IRQn, 1);
    NVIC_EnableIRQ(DMA2_Stream3_IRQn);
}

void SPI_GPIO_Config(void){
        /* Enable GPIOA clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;

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

}