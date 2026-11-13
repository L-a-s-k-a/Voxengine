#include "init.h"
#include "app_data.h"

static G_Mode next_mode(G_Mode m);

/**
 * @brief SysTick = 1 кГц. Тикаем двумя независимыми FSM.
 */
void SysTick_Handler(void)
{
   
}

/**
 * @brief EXTI3 — PIR-датчик (PB3). Сработал по RTSR или FTSR.
 * Читаем реальный уровень IDR и превращаем в pir_sens.
 * Дальше SysTick_Handler сам решит, WORK или CHILL.
 */
void EXTI3_IRQHandler(void)
{
    EXTI->PR = EXTI_PR_PR3; // сброс pending
}

/**
 * @brief EXTI4 — кнопка SETUP (PA4).
 *
 * Пока кнопка нажата (высокий уровень из-за pull-down + замыкания на VCC),
 * settings_btn = SETUP — process.c будет непрерывно читать потенциометр.
 */
void EXTI4_IRQHandler(void)
{
    EXTI->PR = EXTI_PR_PR4;
}

/**
 * @brief EXTI13 — кнопка PC13 (B1 на Nucleo).
 * Здесь только "взводим" FSM: переводим IDLE -> PRESSED и обнуляем
 * таймеры. Дебаунс и LONG обрабатываются в SysTick_Handler.
 * Если FSM не в IDLE (уже считаем или ждём отпускания LONG) — игнорируем.
 */
void EXTI15_10_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR13)
    {
        EXTI->PR = EXTI_PR_PR13;    // сброс pending
    }
}

/**
 * @brief Циклический переход STANDART -> CONTINUOUS -> ENERGY_SAVING -> STANDART.
 * SETTINGS сознательно пропускаем — в него попадают только через кнопку PA4.
 */
static G_Mode next_mode(G_Mode m)
{
    switch (m) {
    case STANDART:      return CONTINUOUS;
    case CONTINUOUS:    return ENERGY_SAVING;
    case ENERGY_SAVING: return STANDART;
    default:            return STANDART;
    }
}