#include "adc1.h"
#include "app_data.h"

/**
 * @brief ADC1 + DMA2_Stream0.
 *
 * Идея: TIM1 генерирует TRGO на каждом Update event (500 Гц).
 * По этому триггеру ADC1 сканирует два канала (0 и 3) и через DMA
 * складывает результаты в g_app.adc_buffer[0..1]. CPU не участвует.
 * В main из этого буфера делает mean_filter() и transform().
 *
 * Настройки ADC:
 *  - 12 бит, right-aligned;
 *  - scan mode (два канала в SQR);
 *  - EXTSEL = TIM1_TRGO, rising.
 *
 * DMA: циклический режим, периферия -> память, 16 бит/16 бит, MINC.
 */
void ADC1_DMA_Config(void){
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;         // DMA2
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;        // PA0, PA3

    /* PA0, PA3 — аналоговый режим (для ADC1_IN0 / ADC1_IN3). */
    GPIOA->MODER |= GPIO_MODER_MODER0 | GPIO_MODER_MODER3; 

    /* Настройка DMA2 Stream0 для ADC1 */
    DMA2_Stream0->CR &= ~DMA_SxCR_EN;           // Выключаем поток перед настройкой
    while (DMA2_Stream0->CR & DMA_SxCR_EN) {}   // Ждём фактического выключения

    DMA2_Stream0->CR = 0;                       // Сброс регистра
    // Канал 0 (CHSEL = 0), направление периферия->память (DIR = 0),
    // размер данных 16 бит (MSIZE = 01, PSIZE = 01),
    // инкремент памяти (MINC = 1), циклический режим (CIRC = 1)
    DMA2_Stream0->CR |= (0U << DMA_SxCR_CHSEL_Pos)   // Channel 0 для ADC1
                     |  (0U << DMA_SxCR_DIR_Pos)     // Peripheral-to-memory
                     |  (1U << DMA_SxCR_MSIZE_Pos)   // 16-bit memory
                     |  (1U << DMA_SxCR_PSIZE_Pos)   // 16-bit peripheral
                     |  (1U << DMA_SxCR_MINC_Pos)    // Memory increment
                     |  (1U << DMA_SxCR_CIRC_Pos);   // Circular mode

    DMA2_Stream0->PAR  = (uint32_t)&(ADC1->DR);                             // Адрес регистра данных ADC
    DMA2_Stream0->M0AR = (uint32_t)(uintptr_t)(void *)&g_app.adc_buffer[0]; // Адрес буфера в памяти
    DMA2_Stream0->NDTR = ADC_BUF_LEN;                                       // Количество передач

    /* Сброс флагов потока перед запуском. */
    DMA2->LIFCR = DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTEIF0 | DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CFEIF0;
    DMA2_Stream0->CR |= DMA_SxCR_EN;                 // Включаем поток

    /* --- Тактирование ADC --- */
    /* ADCPRE = 4: 108 / 4 = 27 МГц. Максимум для ADC на F7 — 36 МГц. */
    ADC->CCR |= ADC_CCR_ADCPRE_0; 
    
    /* --- ADC1 --- */
    ADC1->CR1 = 0;
    ADC1->CR1 &= ~ADC_CR1_RES;              // Resolution 12 bit (RES = 00)
    ADC1->CR1 |= ADC_CR1_SCAN;              // Сканируем несколько каналов

    ADC1->CR2 = 0;
    ADC1->CR2 &= ~ADC_CR2_ALIGN;            // Data right alignment
    ADC1->CR2 &= ~ADC_CR2_CONT;             // Отключение непрерывного сканирования
    ADC1->CR2 |= (9 << ADC_CR2_EXTSEL_Pos); // Начинать преобразования по внешнему триггеру от таймера
    ADC1->CR2 |= ADC_CR2_EXTEN_0;           // Rising trigger on
    ADC1->CR2 |= ADC_CR2_DMA;               // DMA mode enabled
    ADC1->CR2 |= ADC_CR2_DDS;               // DMA-запросы не снимаются между преобразованиями 

    /* Время выборки 56 тактов (SMP=011) — компромисс точность/скорость. */
    ADC1->SMPR2 |= (3 << ADC_SMPR2_SMP0_Pos); 
    ADC1->SMPR2 |= (3 << ADC_SMPR2_SMP3_Pos);

    /* Две конверсии: SQ1=ch0, SQ2=ch3. */
    ADC1->SQR1 = 0;
    ADC1->SQR1 |= ADC_SQR1_L_0;            // L = 1 -> 2 conversion
    ADC1->SQR3 = 0;
    ADC1->SQR3 |= (0 << ADC_SQR3_SQ1_Pos); // SQ1 -> channel 0
    ADC1->SQR3 |= (3 << ADC_SQR3_SQ2_Pos); // SQ2 -> channel 3

    ADC1->CR2 |= ADC_CR2_ADON;                  // ADC on without conversion
    for (volatile int i = 0; i < 1000; i++) {}  // tSTAB — пауза после ADON
}