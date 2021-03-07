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
 * @file app_main.c
 * @brief Example of use of the thermistor controller connected to an analog channel 
 * of the ESP-32. 
 * With menuconfig you can configure the parameters used by the initialization function 
 * to get the instance handle.
 * Every 200 mS is invoked the function that reads the voltage from the resistive 
 * divider and converts it to degrees Celsius using the simplified equation of Steniarth.
 * The temperature is displayed on the monitor in degrees Celsius and Fahrenheit.
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "thermistor.h"

#include "sdkconfig.h"

#include "esp_log.h"
static const char* TAG = "app";

#ifdef CONFIG_IDF_TARGET_ESP32C3
    #include <ws2812_led.h>
    #define DEFAULT_SATURATION  100
    #define DEFAULT_BRIGHTNESS  50
#endif

/**
 * @brief Initialize the led driver to use, if it is an ESP32-C3 development 
 *        kit there are many possibilities that it has a neopixel connected.
 */
static esp_err_t init_led(void)
{
esp_err_t err = ESP_OK;

#ifdef CONFIG_IDF_TARGET_ESP32C3
    err = ws2812_led_init();
    ESP_LOGI(TAG, "ws2812_led_init: %d", err);
#else
    gpio_reset_pin(CONFIG_BLINK_GPIO);
    gpio_set_direction(CONFIG_BLINK_GPIO, GPIO_MODE_INPUT_OUTPUT);
#endif
    return err;
}

/**
 * @brief If the led is neopixel, the color saturation changes with the 
 *        temperature value, if it is a led it toggle each time it is called.
 */
static void show_temp(float temperature)
{
#ifdef CONFIG_IDF_TARGET_ESP32C3
    uint16_t g_hue = (35-(uint16_t)temperature) * 10;
    ws2812_led_set_hsv(g_hue, DEFAULT_SATURATION, DEFAULT_BRIGHTNESS);
#else
    /* Toggle output */
    gpio_set_level(CONFIG_BLINK_GPIO, !gpio_get_level(CONFIG_BLINK_GPIO));
#endif
}

void app_main(void)
{
    thermistor_handle_t th;
    thermistor_init(&th, ADC_CHANNEL_2, 
                    CONFIG_SERIE_RESISTANCE, 
                    CONFIG_NOMINAL_RESISTANCE, 
                    CONFIG_NOMINAL_TEMPERATURE,
                    CONFIG_BETA_VALUE, 
                    CONFIG_VOLTAGE_SOURCE);

    init_led();
 
    while(1) {
        float celsius = thermistor_get_celsius(&th);
        float fahrenheit = thermistor_celsius_to_fahrenheit(celsius);

        ESP_LOGI(TAG,"Voltage: %d mV\tTemperature: %2.1f C / %2.1f F:\tResistance: %.0f ohm", 
                 th.vout, celsius, fahrenheit, th.t_resistance);

        show_temp(celsius);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}
