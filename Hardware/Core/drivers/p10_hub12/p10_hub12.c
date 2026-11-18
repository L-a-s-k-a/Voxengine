#include "p10_hub12.h"


#include "stm32f756xx.h"

/*
 * Clock assumptions:
 *
 * SYSCLK     = 216 MHz
 * APB1       = 54 MHz
 * APB1 timer = 108 MHz
 * APB2       = 108 MHz
 *
 * SPI1 clock = 108 MHz / 32 = 3.375 MHz
 *
 * TIM2: 800 Hz scan interrupt
 * TIM3: 1 MHz timer tick
 */

#define TIM2_CLOCK_HZ       108000000U
#define TIM2_SCAN_HZ        800U

#define TIM3_CLOCK_HZ       108000000U
#define TIM3_TICK_HZ        1000000U

#define SPI_DELAY_US        10U

#define LED_DMA_STREAM      DMA2_Stream3

#define LED_LAT_PIN         0U
#define LED_OE_PIN          1U
#define LED_A_PIN           2U
#define LED_B_PIN           10U

#define GPIO_SET(port, pin) \
    ((port)->BSRR = (1U << (pin)))

#define GPIO_RESET(port, pin) \
    ((port)->BSRR = (1U << ((pin) + 16U)))

/*
 * DMA buffer.
 *
 * Place this section in DMA-accessible SRAM1.
 * Do not place it in DTCM.
 */
__attribute__((section(".dma_buffer")))
__attribute__((aligned(32)))
static volatile uint8_t dma_buffer[LED_ROW_BYTES];

static led_frame_t frame_buffer;

static volatile uint8_t current_row = 0;
static volatile uint8_t transfer_busy = 0;


/* ---------------------------------------------------------
 * SPI1
 * --------------------------------------------------------- */

static void LED_SPI_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    SPI1->CR1 = 0;
    SPI1->CR2 = 0;

    /*
     * Master
     * Baud rate: PCLK2 / 32
     * CPOL = 0
     * CPHA = 0
     * MSB first
     * Software NSS
     * 8-bit data
     * Bidirectional transmit only
     */

    SPI1->CR1 =
        SPI_CR1_MSTR |
        SPI_CR1_BR_2 |
        SPI_CR1_BR_0 |
        SPI_CR1_SSM |
        SPI_CR1_SSI |
        SPI_CR1_BIDIMODE |
        SPI_CR1_BIDIOE;

    SPI1->CR2 = 0;

    SPI1->CR1 |= SPI_CR1_SPE;
}


/* ---------------------------------------------------------
 * DMA2 Stream 3
 * --------------------------------------------------------- */

static void LED_DMA_Init(void)
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


/* ---------------------------------------------------------
 * TIM2: periodic scan timer
 * --------------------------------------------------------- */

static void LED_TIM2_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2->CR1 = 0;

    TIM2->PSC = 0;

    TIM2->ARR =
        (TIM2_CLOCK_HZ / TIM2_SCAN_HZ) - 1U;

    TIM2->CNT = 0;

    TIM2->DIER = TIM_DIER_UIE;

    TIM2->EGR = TIM_EGR_UG;

    TIM2->SR = 0;

    NVIC_SetPriority(TIM2_IRQn, 2);
    NVIC_EnableIRQ(TIM2_IRQn);
}


/* ---------------------------------------------------------
 * TIM3: one-shot delay after SPI DMA
 * --------------------------------------------------------- */

static void LED_TIM3_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    TIM3->CR1 = 0;

    TIM3->PSC =
        (TIM3_CLOCK_HZ / TIM3_TICK_HZ) - 1U;

    TIM3->ARR = SPI_DELAY_US - 1U;

    TIM3->CNT = 0;

    /*
     * One-pulse mode.
     */

    TIM3->CR1 |= TIM_CR1_OPM;

    TIM3->DIER = TIM_DIER_UIE;

    TIM3->EGR = TIM_EGR_UG;

    TIM3->SR = 0;

    NVIC_SetPriority(TIM3_IRQn, 1);
    NVIC_EnableIRQ(TIM3_IRQn);
}


/* ---------------------------------------------------------
 * Public initialization
 * --------------------------------------------------------- */

void LED_Init(void)
{
    LED_GPIO_Init();

    LED_SPI_Init();

    LED_DMA_Init();

    LED_TIM2_Init();

    LED_TIM3_Init();

    current_row = 0;

    transfer_busy = 0;
}


/* ---------------------------------------------------------
 * Start scanning
 * --------------------------------------------------------- */

void LED_Start(void)
{
    TIM2->CNT = 0;

    TIM2->CR1 |= TIM_CR1_CEN;
}


/* ---------------------------------------------------------
 * Frame update
 * --------------------------------------------------------- */

void LED_SetFrame(const led_frame_t *frame)
{
    if (frame == 0)
        return;

    /*
     * For initial testing only.
     * Later replace with double buffering.
     */

    for (uint32_t i = 0; i < LED_SCAN_ROWS; i++)
    {
        for (uint32_t j = 0; j < LED_ROW_BYTES; j++)
        {
            frame_buffer.row[i][j] =
                frame->row[i][j];
        }
    }
}


/* ---------------------------------------------------------
 * Start one DMA transfer
 * --------------------------------------------------------- */

static void LED_StartTransfer(void)
{
    if (transfer_busy)
        return;

    transfer_busy = 1;

    /* Disable display while shifting data */

    GPIO_SET(GPIOB, LED_OE_PIN);

    /* Copy current row into DMA buffer */

    for (uint32_t i = 0; i < LED_ROW_BYTES; i++)
    {
        dma_buffer[i] =
            frame_buffer.row[current_row][i];
    }

    /*
     * If D-cache is enabled, clean this range here:
     *
     * SCB_CleanDCache_by_Addr(
     *     (uint32_t *)dma_buffer,
     *     32
     * );
     *
     * Ensure the address and length meet the
     * cache-line alignment requirements.
     */

    __DSB();

    DMA2->LIFCR =
        DMA_LIFCR_CTCIF3 |
        DMA_LIFCR_CTEIF3 |
        DMA_LIFCR_CDMEIF3 |
        DMA_LIFCR_CFEIF3;

    LED_DMA_STREAM->M0AR =
        (uint32_t)dma_buffer;

    LED_DMA_STREAM->NDTR =
        LED_ROW_BYTES;

    /* Enable SPI TX DMA request */

    SPI1->CR2 |= SPI_CR2_TXDMAEN;

    /* Start DMA */

    LED_DMA_STREAM->CR |= DMA_SxCR_EN;
}


/* ---------------------------------------------------------
 * TIM2 IRQ
 * --------------------------------------------------------- */

void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF)
    {
        TIM2->SR &= ~TIM_SR_UIF;

        LED_StartTransfer();
    }
}


/* ---------------------------------------------------------
 * DMA transfer complete IRQ
 * --------------------------------------------------------- */

void DMA2_Stream3_IRQHandler(void)
{
    uint32_t status = DMA2->LISR;

    if (status & DMA_LISR_TCIF3)
    {
        DMA2->LIFCR = DMA_LIFCR_CTCIF3;

        SPI1->CR2 &= ~SPI_CR2_TXDMAEN;

        /*
         * DMA finished writing SPI_DR.
         * SPI may still be transmitting.
         *
         * Start TIM3 to wait for the last bits.
         */

        TIM3->CR1 &= ~TIM_CR1_CEN;

        TIM3->CNT = 0;

        TIM3->SR = 0;

        TIM3->CR1 |= TIM_CR1_CEN;
    }

    if (status & (
        DMA_LISR_TEIF3 |
        DMA_LISR_DMEIF3 |
        DMA_LISR_FEIF3
    ))
    {
        DMA2->LIFCR =
            DMA_LIFCR_CTEIF3 |
            DMA_LIFCR_CDMEIF3 |
            DMA_LIFCR_CFEIF3;

        SPI1->CR2 &= ~SPI_CR2_TXDMAEN;

        LED_DMA_STREAM->CR &= ~DMA_SxCR_EN;

        transfer_busy = 0;
    }
}


/* ---------------------------------------------------------
 * TIM3 IRQ
 * --------------------------------------------------------- */

void TIM3_IRQHandler(void)
{
    if (TIM3->SR & TIM_SR_UIF)
    {
        TIM3->SR &= ~TIM_SR_UIF;

        /*
         * At this point SPI should have finished
         * transmitting all 16 bytes.
         */

        /* Latch pulse */

        GPIO_SET(GPIOB, LED_LAT_PIN);

        __NOP();
        __NOP();

        GPIO_RESET(GPIOB, LED_LAT_PIN);

        /* Select next row */

        if (current_row & 0x01U)
            GPIO_SET(GPIOB, LED_A_PIN);
        else
            GPIO_RESET(GPIOB, LED_A_PIN);

        if (current_row & 0x02U)
            GPIO_SET(GPIOB, LED_B_PIN);
        else
            GPIO_RESET(GPIOB, LED_B_PIN);

        /* Enable display */

        GPIO_RESET(GPIOB, LED_OE_PIN);

        current_row++;

        if (current_row >= LED_SCAN_ROWS)
            current_row = 0;

        transfer_busy = 0;
    }
}
