#include "sys_tim.h"

/**
 * @brief SysTick на 1 кГц — "сердце" всей временной логики.
 *
 * В SysTick_Handler обрабатываются:
 *  - дебаунс/удержание кнопки PC13 (FSM);
 *  - отсчёт времени отсутствия пассажиров PIR.
 *
 * LOAD = 216000 - 1 = 215999, источник — AHB без деления.
 */
void SysTick_Init_Config(void)
{
    CLEAR_BIT(SysTick->CTRL, SysTick_CTRL_ENABLE_Msk);  // На всякий случай, предварительно, выключим счётчик
    SET_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);   // Разрешаем прерывание по системному таймеру
    SET_BIT(SysTick->CTRL, SysTick_CTRL_CLKSOURCE_Msk); // Источник тактирования будет идти из AHB без деления
    MODIFY_REG(SysTick->LOAD, SysTick_LOAD_RELOAD_Msk,
               215999 << SysTick_LOAD_RELOAD_Pos);      // Значение с которого начинается счёт, эквивалентное 1 кГц
    CLEAR_REG(SysTick->VAL);     // Очистка поля
    SET_BIT(SysTick->CTRL, SysTick_CTRL_ENABLE_Msk);    // Включим счётчик

    NVIC_SetPriority(SysTick_IRQn, 4);
}