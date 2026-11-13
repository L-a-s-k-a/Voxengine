#ifndef PROCESS_H
#define PROCESS_H

#include "app_data.h"

/* --- Абсолютные границы мощности (в процентах). --- */
#define MAXIMUM_POWER       100
#define MINIMUM_POWER       5

/* --- Уставки по умолчанию для каждого режима (в процентах). --- */
/* STANDART: две точки — "есть люди" и "нет людей". */
#define STANDART_MAX_PWR    90
#define STANDART_MIN_PWR    10
/* ENERGY_SAVING: ограничения для значения с потенциометра,
 * чтобы мотор не встал и не перегрелся. */
#define ENRG_SAV_MAX_PWR    40
#define ENRG_SAV_MIN_PWR    5

/* Главная функция: по текущему режиму выставляет g_app.pulse. */
void mode_selection(G_Mode mode);

/* Устанавливает g_app.pulse из процента (с насыщением 0..100). */
void set_power(uint8_t power);

/* Читает канал АЦП, фильтрует и переводит в проценты (с учётом режима). */
uint16_t transform(uint8_t channel, G_Mode mode);

/* Скользящее среднее по FILTER_N отсчётам для указанного канала. */
void mean_filter(uint8_t channel);

#endif