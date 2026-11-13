#include "init.h"
#include "app_data.h"

static G_Mode next_mode(G_Mode m);

/**
 * @brief SysTick = 1 кГц. Тикаем двумя независимыми FSM.
 *
 * 1) FSM кнопки PC13.
 *    - BTN_IDLE: ничего не делаем, ждём, пока EXTI15_10 переведёт
 *      в BTN_PRESSED.
 *    - BTN_PRESSED: сначала набираем BTN_DEBOUNCE_MS стабильного
 *      нажатия. После этого каждый тик увеличиваем btn_hold_ms.
 *      Если за это время кнопку отпустили — SHORT -> сменить режим.
 *      Если добрали до BTN_LONG_PRESS_MS — LONG -> переключить питание,
 *      перейти в BTN_LONG_FIRED.
 *    - BTN_LONG_FIRED: ждём отпускания, чтобы не сработало второе
 *      "короткое" нажатие при отрыве пальца.
 *
 * 2) PIR-таймер.
 *    pir_sens ставится в EXTI3. Здесь из него получаем pir_state:
 *    - если pir_sens == DETECTED: WORK, сбрасываем pir_time;
 *    - если NO_DETECTED и pir_time >= DETECTING_TIME: CHILL.
 */
void SysTick_Handler(void)
{
    bool pin_pressed = (GPIOC->IDR & GPIO_IDR_ID13) != 0; // 1 - нажата

    switch (g_app.btn_state)
    {
    case BTN_IDLE:
        break;

    case BTN_PRESSED:
        if (++g_app.btn_debounce_ms < BTN_DEBOUNCE_MS) break; // ждём дебаунс
        if (pin_pressed)
        {
            if (++g_app.btn_hold_ms >= BTN_LONG_PRESS_MS)
            {
                // LONG: переключаем питание — режим НЕ трогаем
                g_app.pwr_status = (g_app.pwr_status == PWR_ON) ? PWR_OFF : PWR_ON;
                g_app.btn_state = BTN_LONG_FIRED;
            }
        }
        else
        {
            // SHORT: отпустили, не дойдя до LONG — меняем режим
            g_app.mode_btn = next_mode(g_app.mode_btn);
            g_app.btn_state = BTN_IDLE;
        }
        break;

    case BTN_LONG_FIRED:
        // Ждём отпускания, ничего больше не делаем
        if (!pin_pressed) g_app.btn_state = BTN_IDLE;
        break;
    }

    /* PIR-таймер: 3 с без движения -> CHILL. */
    if (g_app.pir_sens == NO_DETECTED)
    {
        if (++g_app.pir_time >= DETECTING_TIME)
        {
            g_app.pir_state = CHILL;
            g_app.pir_time = 0U;
        }
    }
    else
    {
        /* Движение есть — сразу WORK и обнуляем счётчик.
         * Если датчик не отпустит DETECTED, WORK будет держаться
         * постоянно (это ожидаемо для PIR в активной зоне). 
         */
        g_app.pir_state = WORK;
        g_app.pir_time = 0U;
    }
}

/**
 * @brief EXTI3 — PIR-датчик (PB3). Сработал по RTSR или FTSR.
 * Читаем реальный уровень IDR и превращаем в pir_sens.
 * Дальше SysTick_Handler сам решит, WORK или CHILL.
 */
void EXTI3_IRQHandler(void)
{
    EXTI->PR = EXTI_PR_PR3; // сброс pending
    g_app.pir_sens = (GPIOB->IDR & GPIO_IDR_ID3) ? DETECTED : NO_DETECTED;
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
    g_app.settings_btn = (GPIOA->IDR & GPIO_IDR_ID4) ? SETUP : NO_SETUP;
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
        if (g_app.btn_state == BTN_IDLE)
        {
            g_app.btn_state = BTN_PRESSED;
            g_app.btn_hold_ms = 0;
            g_app.btn_debounce_ms = 0;
        }
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