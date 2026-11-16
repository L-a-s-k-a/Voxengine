#ifndef HUB12_H
#define HUB12_H

#include <stdint.h>
#include <stdbool.h>

#define HUB12_WIDTH       32U
#define HUB12_HEIGHT      16U

#define HUB12_BUFFER_SIZE \
    ((HUB12_WIDTH * HUB12_HEIGHT) / 8U)

void HUB12_Init(void);

void HUB12_SetPixel(
    uint8_t x,
    uint8_t y,
    bool state
);

void HUB12_Clear(void);

void HUB12_Refresh(void);

#endif