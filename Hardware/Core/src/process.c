#include "process.h"

/**
 * @brief Главный "мозг": выбрать мощность по режиму и состоянию.
 *
 * STANDART:
 *   PIR говорит WORK  -> STANDART_MAX_PWR (90%).
 *   PIR говорит CHILL -> STANDART_MIN_PWR (10%).
 *   Потенциометры не участвуют.
 *
 * CONTINUOUS:
 *   Если зажата кнопка SETUP — читаем потенциометр (канал 0),
 *   сохраняем в eng_pwr.eng_pwr_cont_mode.
 *   Всегда применяем это значение (даже если потенциометр двигают
 *   без SETUP, значение не поменяется — оно "залипло" в eng_pwr).
 *
 * ENERGY_SAVING:
 *   Аналогично, но канал 1. Дополнительно жёстко ограничиваем
 *   результат в [ENRG_SAV_MIN_PWR, ENRG_SAV_MAX_PWR],
 *   чтобы независимо от потенциометра мотор не вышел за допустимые
 *   для этого режима обороты.
 *
 * default:
 *   Ничего не делаем
 */
void mode_selection(G_Mode mode)
{
    switch (mode)
    {
    case STANDART:
        if (g_app.pir_state == WORK)
        {
            set_power(STANDART_MAX_PWR);
        }
        else if (g_app.pir_state == CHILL)
        {
            set_power(STANDART_MIN_PWR);
        }

        g_app.mode_display = STANDART;
        break;

    case CONTINUOUS:
        if (g_app.settings_btn == SETUP) eng_pwr.eng_pwr_cont_mode = transform(0, CONTINUOUS);
        set_power(eng_pwr.eng_pwr_cont_mode);

        g_app.mode_display = CONTINUOUS;
        break;

    case ENERGY_SAVING:
        if (g_app.settings_btn == SETUP) eng_pwr.eng_pwr_enrg_save_mode = transform(1, ENERGY_SAVING);
        /* Двойная защита: насыщение уже в set_power, но здесь — ещё
         * тематический коридор для энергосберегающего режима. */
        if (eng_pwr.eng_pwr_enrg_save_mode > ENRG_SAV_MAX_PWR) eng_pwr.eng_pwr_enrg_save_mode = ENRG_SAV_MAX_PWR;
        else if (eng_pwr.eng_pwr_enrg_save_mode < ENRG_SAV_MIN_PWR) eng_pwr.eng_pwr_enrg_save_mode = ENRG_SAV_MIN_PWR;
        
        set_power(eng_pwr.eng_pwr_enrg_save_mode);

        g_app.mode_display = ENERGY_SAVING;
        break;

    default:
        break;
    }
}

/**
 * @brief Процент -> значение CCR1.
 *
 * pulse = power% * ARR / 100, ограничено сверху ARR-1.
 * Ограничение ARR-1 — чтобы CCR не догнал ARR и не было 100%-скважности
 * "в упор". На практике разница в 1 шаг из 1000 незаметна, но
 * защищает от крайних случаев драйвера.
 */
void set_power(uint8_t power)
{
    /*---Ограничение, в случае некорректного задания мощности---*/
    if (power > MAXIMUM_POWER)      power = MAXIMUM_POWER;
    else if (power < MINIMUM_POWER) power = MINIMUM_POWER;

    g_app.pulse = (uint16_t)(((float)power * (float)init_config.ARR) / 100.0f);
    g_app.pulse = (g_app.pulse > (init_config.ARR - 1)) ? (init_config.ARR - 1) : g_app.pulse;
}

/**
 * @brief АЦП -> процент.
 *
 * 1. Прогоняем mean_filter, чтобы сгладить шум потенциометра.
 * 2. filtered / 4095 * 100 -> процент (0..100).
 * 3. В ENERGY_SAVING делим на 2 (уставка с потенциометра
 *    дополнительно режется вдвое — "энергосбережение").
 */
uint16_t transform(uint8_t channel, G_Mode mode)
{
    mean_filter(channel);
    float pct = (100.0f * g_app.filtered_ch[channel]) / 4095.0f;
    if (mode == ENERGY_SAVING) pct /= 2.0f;
    return (uint16_t)pct;
}

/**
 * @brief Блочное усреднение по FILTER_N (нестрогое — не по кольцу,
 *        а "сумма за окно / счётчик", что эквивалентно среднему за окно).
 *
 * Идея:
 *   - аккумулируем sum[channel] += новое значение;
 *   - как только набрали FILTER_N отсчётов, публикуем среднее
 *     в g_app.filtered_ch[channel] и обнуляем аккумулятор.
 *
 * Следствия:
 *   - filtered_ch обновляется "пачками" раз в FILTER_N вызовов;
 *   - до первого обновления filtered_ch = 0 -> transform вернёт 0.
 *
 * static-переменные — потому что состояние фильтра приватное
 * и не нужно никому снаружи. По одному аккумулятору на канал.
 */
void mean_filter(uint8_t channel)
{
    static uint32_t sum[2];
    static uint16_t cnt[2];

    sum[channel] += g_app.adc_buffer[channel];
    if (++cnt[channel] >= FILTER_N) {
        g_app.filtered_ch[channel] = (float)sum[channel] / cnt[channel];
        sum[channel] = 0;
        cnt[channel] = 0;
    }
}