#include "p10_hub12.h"

static uint8_t framebuffer[HUB12_BUFFER_SIZE];

void HUB12_SetPixel(uint8_t x, uint8_t y, bool state)
{
    if ((x >= HUB12_WIDTH) || (y >= HUB12_HEIGHT))
    {
        return;
    }

    uint16_t index = (uint16_t)y * HUB12_WIDTH + x;

    uint16_t byte_index = index / 8U;
    uint8_t bit_index = index % 8U;

    if (state)
    {
        framebuffer[byte_index] |= (1U << bit_index);
    }
    else
    {
        framebuffer[byte_index] &= ~(1U << bit_index);
    }
}

void HUB12_Clear(void)
{
    for (uint32_t i = 0U; i < HUB12_BUFFER_SIZE; i++)
    {
        framebuffer[i] = 0U;
    }
}