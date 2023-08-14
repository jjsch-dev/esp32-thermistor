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

#include "math.h"

#include "esp_log.h"
static const char* TAG = "drv_thr";

#define DEFAULT_VREF    1100        // Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          // Amount suggested by espresif for multiple samples.

static bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle);

esp_err_t thermistor_init(thermistor_handle_t* th,
                          adc_channel_t channel, float serial_resistance, 
                          float nominal_resistance, float nominal_temperature, 
                          float beta_val, float vsource)
{
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };

    esp_err_t err = adc_oneshot_new_unit(&init_config, &adc_handle);
    
    if (err == ESP_OK) {
        adc_oneshot_chan_cfg_t config = {
                    .bitwidth = ADC_BITWIDTH_12, 
                    .atten = ADC_ATTEN_DB_11,
        };
        
        err = adc_oneshot_config_channel(adc_handle, channel, &config);

        adc_cali_handle_t adc_cali_handle = NULL;
        th->calibrated = adc_calibration_init(ADC_UNIT_1, ADC_ATTEN_DB_11, &adc_cali_handle);
        th->channel = channel;
        th->adc_h = adc_handle;
        th->adc_cali_h = adc_cali_handle;
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
int adc_raw;
int voltage = 0;
esp_err_t err;
   
double sum = 0.0f;
double c = 0.0f; // Variable to store the error
double y;
double t;
int i;

   // Use multiple samples to stabilize the measured value, and 
   // implement the Kahan summation algorithm to reduce the int error.
   for (i = 0; i < NO_OF_SAMPLES; i++) {
      err = adc_oneshot_read(th->adc_h, th->channel, &adc_raw);
      
      if(err != ESP_OK) {
         break;
      }

      y = adc_raw - c;
      t = sum + y;
         
      // Algebraically, c is always 0
      // when t is replaced by its
      // value from the above expression.
      // But, when there is a loss,
      // the higher-order y is cancelled
      // out by subtracting y from c and
      // all that remains is the
      // lower-order error in c
      c = (t - sum) - y;
      sum = t;
   }
   
   adc_raw = (int)(sum/i);
     
   if ((err== ESP_OK) && (th->calibrated)) {
      adc_cali_raw_to_voltage(th->adc_cali_h, adc_raw, &voltage);
   }
   
   return voltage;
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

static bool adc_calibration_init(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}
