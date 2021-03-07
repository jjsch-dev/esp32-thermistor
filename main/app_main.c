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
#include <stdio.h>
#include <stdatomic.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "thermistor.h"

#include "sdkconfig.h"

#ifdef CONFIG_IDF_TARGET_ESP32C3
    #include <ws2812_led.h>
    #define DEFAULT_SATURATION  100
    #define DEFAULT_BRIGHTNESS  50
#endif

//#include "esp_log.h"
//static const char* TAG = "thr";

/* Can use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO

static esp_err_t init_led(void)
{
esp_err_t err = ESP_OK;

#ifdef CONFIG_IDF_TARGET_ESP32C3
    err = ws2812_led_init();
    printf("ws2812_led_init: %d\n", err);
#else
    /* Configure the IOMUX register for pad BLINK_GPIO (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
#endif
    return err;
}

static void show_temp(float temperature)
{
#ifdef CONFIG_IDF_TARGET_ESP32C3
    uint16_t g_hue = (35-(uint16_t)temperature) * 10;
    ws2812_led_set_hsv(g_hue, DEFAULT_SATURATION, DEFAULT_BRIGHTNESS);
#else
    /* Toggle output */
    gpio_set_level(BLINK_GPIO, !gpio_get_level(BLINK_GPIO));
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
        uint32_t vout = thermistor_read_vout(&th);
        float temperature = thermistor_vout_to_temp(&th, vout);
        printf("Voltage: %dmV\tTemperature: %2.1f C\tResistance: %.0f\n", vout, temperature, th.t_resistance);

        show_temp(temperature);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}
