/*
 * HackQuad - an open-source firmware+hardware quadcopter
 * Copyright (C) 2020, Andrew Howard, <divisionind.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "battery.h"

#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "motor.h"

#define BATTERY_MULTISAMPLES 4
#define BATTERY_VDIV_R1      150000.f
#define BATTERY_VDIV_R2      470000.f
/* vdrop mosfet */
#define BATTERY_OFFSET       0.688f
#define BATTERY_SIZE_MA      350.f
#define BATTERY_SIZE_MAS     ((BATTERY_SIZE_MA) * 3600.f)

static esp_adc_cal_characteristics_t battery_adc_chars;

void battery_init() {
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF))
        ESP_LOGW(TAG, "eFuse Vref NOT found. Battery readings may be inaccurate.");

    // TODO: disable this when plugged in to usb to fix weird charge-controller bug (coil wine and flickering)
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_2_5 /* range 100-1250 mV */));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_2_5, ADC_WIDTH_BIT_12, 1100, &battery_adc_chars);

    ESP_LOGI(TAG, "Using Vref: %i mV", battery_adc_chars.vref);
}

static u32 battery_read_raw() {
    int i;
    u32 total = 0;

    for (i = 0; i < BATTERY_MULTISAMPLES; i++)
        total += adc1_get_raw(/* GPIO37 */ ADC1_CHANNEL_1);

    return total / BATTERY_MULTISAMPLES;
}

// on second thought.. i dont want to figure out their vref calibration and this is good enough (and this should use dma)
/*static u32 battery_read_raw_oversample(const u32 n) {
    u32 ret = 0;

    for (u32 i = 0; i < (1 << 2*n); i++)
        ret += adc1_get_raw(*//* GPIO37 *//* ADC1_CHANNEL_1);

    return ret >> n;
}*/

static u32 battery_calc_mv(u32 raw) {
    return esp_adc_cal_raw_to_voltage(raw, &battery_adc_chars);
}


float battery_read() {
    return ((((float) battery_calc_mv(battery_read_raw()) / 1000.f) * (BATTERY_VDIV_R1 + BATTERY_VDIV_R2)) / BATTERY_VDIV_R1) + BATTERY_OFFSET;
}

/**
 * Estimate current draw from battery.
 *
 * @param V bat voltage
 * @return mA
 */
/*
float battery_estimate_current(float V) {
    u32 motors = 0;
    float ret = 0.f;

    for (unsigned int i = 0; i < (M3+1); i++)
        motors += motor_throttle_get(i);

    //
    return ret;
}*/
