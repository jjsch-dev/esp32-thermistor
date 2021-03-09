/*
 * MIT License
 * 
 * Copyright (c) 2021 Juan Schiavoni
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file thermistor.c
 * @brief Driver implementation of thermistor component for ESP32.
 */

#include "thermistor.h"

#include "driver/gpio.h"
#include "esp_adc_cal.h"
#include "math.h"

#include "esp_log.h"
static const char* TAG = "drv_thr";

#define DEFAULT_VREF    1100        // Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          // Amount suggested by espresif for multiple samples.

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
static const adc_atten_t atten = ADC_ATTEN_DB_11;

/**
 * @brief Checks if ADC calibration values are burned into eFuse.
 * @param[in] void .
 * @return void
 */
static void check_efuse(void)
{
    // Check if TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        ESP_LOGI(TAG, "eFuse Two Point: Supported");
    } else {
        ESP_LOGI(TAG, "Cannot retrieve eFuse Two Point calibration values. Default calibration values will be used.");
    }
#if CONFIG_IDF_TARGET_ESP32 
    // Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        ESP_LOGI(TAG, "eFuse Vref: Supported");
    } else {
        ESP_LOGI(TAG, "eFuse Vref: NOT supported");
    }
#endif

#if !CONFIG_IDF_TARGET_ESP32 && !CONFIG_IDF_TARGET_ESP32S2 && !CONFIG_IDF_TARGET_ESP32C3
    #error "For now only support: ESP32/ESP32S2/ESP32C3."
#endif
}

/**
 * @brief Log the characterization used to linearize the ADC.
 * @param[in] val_type Type of calibration value used in characterization.
 * @return void
 */
static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        ESP_LOGI(TAG, "Characterized using Two Point Value");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        ESP_LOGI(TAG, "Characterized using eFuse Vref");
    } else {
        ESP_LOGI(TAG, "Characterized using Default Vref");
    }
}

esp_err_t thermistor_init(thermistor_handle_t* th,
                          adc_channel_t channel, float serial_resistance, 
                          float nominal_resistance, float nominal_temperature, 
                          float beta_val, float vsource)
{
    check_efuse();

    esp_err_t err = adc1_config_channel_atten(channel, atten);
    
    if (err == ESP_OK) {
        // Characterize ADC
        adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
        esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, atten, width, DEFAULT_VREF, adc_chars);
        print_char_val_type(val_type);

        ESP_LOGI(TAG, "Vref: %dmV", adc_chars->vref);

        th->channel = channel;
        th->serial_resistance = serial_resistance; 
        th->nominal_resistance = nominal_resistance;
        th->nominal_temperature = nominal_temperature;
        th->beta_val = beta_val;
        th->vsource = vsource;
        th->t_resistance = 0;
    }
    
    return err;
}

float thermistor_vout_to_celsius(thermistor_handle_t* th, uint32_t vout)
{
    float steinhart;
       
    // Rt = R1 * Vout / (Vs - Vout);
    th->t_resistance =  (th->serial_resistance * vout) / (th->vsource - vout); 

    steinhart = th->t_resistance / th->nominal_resistance;  // (R/Ro)
    steinhart = log(steinhart);                             // ln(R/Ro)
    steinhart /= th->beta_val;                              // 1/B * ln(R/Ro)
    steinhart += 1.0 / (th->nominal_temperature + 273.15);  // + (1/To)
    steinhart = 1.0 / steinhart;                            // Invert
    steinhart -= 273.15;                                    // convert to C
 
    return steinhart; 
}

uint32_t thermistor_read_vout(thermistor_handle_t* th)
{
    uint32_t adc_reading = 0;
    
    // Use multiple samples to stabilize the measured value.
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        adc_reading += adc1_get_raw((adc1_channel_t)th->channel);
    }
    adc_reading /= NO_OF_SAMPLES;
    
    return esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
}

float thermistor_get_celsius(thermistor_handle_t* th)
{
    th->vout = thermistor_read_vout(th);
    
    return thermistor_vout_to_celsius(th, th->vout);
}

float thermistor_celsius_to_fahrenheit(float temp)
{
    return (temp * 1.8) + 32;
}
