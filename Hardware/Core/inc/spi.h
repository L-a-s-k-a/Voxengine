#ifndef SPI_H
#define SPI_H

#include "init.h"

void SPI1_Init(void);

void SPI1_TransmitByte(uint8_t data);

void SPI1_TransmitBuffer(
    const uint8_t *data,
    uint32_t length
);

#endif