#ifndef SPI_H
#define SPI_H

#include "stm32f7xx.h"

#include <stdint.h>

#define LED_SCAN_ROWS  4
#define LED_ROW_BYTES  16

#define DMA_SPI_BUFFER_SIZE 2

#define DMA_STREAM      DMA2_Stream3

typedef struct
{
    uint8_t row[LED_SCAN_ROWS][LED_ROW_BYTES];

} led_frame_t;

void SPI1_Config(void);
void SPI_GPIO_Config(void);
void SPI_DMA_Init(void);
void SPI1_Init(void);

#endif