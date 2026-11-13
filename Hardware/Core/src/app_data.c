#include "app_data.h"

/* Единственный экземпляр глобального состояния.
 * volatile-поля не инициализируем — они стартуют с 0 (BSS) или
 * перезаписываются прерываниями в первые же мс. */
AppData_t g_app =
{
    /* pulse = 500 при ARR=1000 -> 50% скважности "на всякий случай",
     * пока mode_selection ещё не отработал ни разу. */
    .pulse = 500,

    .mode_btn = STANDART,
    .btn_state = BTN_IDLE,
    .settings_btn = NO_SETUP,

    .pir_sens = NO_DETECTED,
    .pir_state = CHILL,
    .pir_time = 0,

    .pwr_status = PWR_ON,
    .mode_display = STANDART
};

/* Значения по умолчанию для TIM1. Дублируются в main() — на случай,
 * если main забудут обновить. */
InitConfig init_config = {
    .PSC = 216,     // 108 МГц / 216 = 1000 кГц тик таймера
    .ARR = 1000,    // 1000 кГц / 1000 = 1 кГц ШИМ
};

/* Уставки скорости по умолчанию (пока потенциометры не трогали):
 *  - непрерывный режим: 90%;
 *  - энергосберегающий: 30%. */
EnginePwr eng_pwr = {
    .eng_pwr_cont_mode = 90,
    .eng_pwr_enrg_save_mode = 30,
};