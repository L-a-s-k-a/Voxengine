#include "init.h"
#include "process.h"
#include "app_data.h"

/**
 * @brief Точка входа.
 *
 * Общий поток:
 *   1. Инициализация периферии (порядок важен).
 *   2. В бесконечном цикле:
 *      - рисуем LED по текущему режиму и питанию;
 *      - вызываем mode_selection() — она решает, какая мощность нужна;
 *      - пишем g_app.pulse в TIM1->CCR1;
 *      - уходим в __WFI до следующего прерывания.
 *
 * Всё "живое" обновление состояния (pir_state, btn_state, pir_time,
 * adc_buffer) делают прерывания: SysTick, EXTI, DMA. main только
 * потребляет результат. Это классический event-driven с sleep.
 */
int main(void)
{
    /* Порядок инициализации критичен:
     *  - PSC/ARR нужны до TIM1_Config;
     *  - SystemClock — до SysTick (LOAD считается от 216 МГц);
     *  - TIM1_Config — до ADC1_DMA_Config (ADC триггерится от TIM1_TRGO). 
     */
    init_config.PSC = 216;
    init_config.ARR = 1000;
    SystemClock_Config();
    SysTick_Init_Config();
    GPIO_Config();
    EXTI_Config();
    TIM1_Config();
    ADC1_DMA_Config();

    while (1)
    {
        if (g_app.pwr_status == PWR_ON)
        {
            /* --- Индикация режима: три LED на PC8..PC10. --- */
            if (g_app.mode_btn == STANDART)
            {
                STANDART_LED_ON;
                CONTIN_LED_OFF;
                ENRG_SAV_LED_OFF;
            }

            if (g_app.mode_btn == CONTINUOUS)
            {
                STANDART_LED_OFF;
                CONTIN_LED_ON;
                ENRG_SAV_LED_OFF;
            }

            if (g_app.mode_btn == ENERGY_SAVING)
            {
                STANDART_LED_OFF;
                CONTIN_LED_OFF;
                ENRG_SAV_LED_ON;
            }

            PWR_LED_OFF; // питание включено — индикатор "PWR" не горит

            /* --- Расчёт мощности: mode_selection читает PIR/потенциометры
             *     и выставляет g_app.pulse. --- */
            mode_selection(g_app.mode_btn);

            /* --- Записываем скважность в CCR1 (preload -> применится на UEV). --- */
            TIM1->CCR1 = g_app.pulse;

            /* DSB — гарантия, что все записи в периферию завершены
             * до того, как ядро уснёт. Иначе есть теоретический шанс,
             * что WFI "обгонит" запись в CCR1. 
             */
            __DSB(); // Все pending-записи в память/периферию завершены, прежде чем уснуть
            __WFI(); // уснуть до следующего прерывания (SysTick или TIM1)
        }
        else
        {
            /* --- PWR_OFF: гасим всё, что можно, кроме PWR_LED. --- */
            PWR_LED_ON;
            STANDART_LED_OFF;
            CONTIN_LED_OFF;
            ENRG_SAV_LED_OFF;
            TIM1->CCR1 = 0;     // двигатель стоит

            __DSB(); // Все pending-записи в память/периферию завершены, прежде чем уснуть
            __WFI(); // уснуть до следующего прерывания (SysTick или TIM1)
        }
    }
}