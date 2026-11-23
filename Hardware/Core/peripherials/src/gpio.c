#include "gpio.h"

void GPIO_Config(void){
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOGEN;

    GPIOG->MODER |= GPIO_MODER_MODER10_0 | 
                    GPIO_MODER_MODER11_0 |
                    GPIO_MODER_MODER12_0 |
                    GPIO_MODER_MODER13_0;
    GPIOG->BSRR = 0U;
}

