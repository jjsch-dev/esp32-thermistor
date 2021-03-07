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

#ifndef __THERMISTOR_H__
#define __THERMISTOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/adc.h"

/**
 * @brief Structure to storing the thermistor instance.
 *
 * @note Call thermistor_init() to initialize the structure
 */
typedef struct  
{
    adc_channel_t channel;          /**< ADC channel pin where the thermistor is connected*/
    float serial_resistance;        /**< Value of the serial resistor connected to +3V*/
    float nominal_resistance;       /**< Nominal resistance at 25 degrees Celsius of thermistor*/
    float nominal_temperature;      /**< Nominal temperature of the thermistor, usually 25 degress Celsius*/
    float beta_val;                 /**< Beta coefficient of the thermistor*/
    float vsource;                  /**< Voltage to which the serial resistance is connected in mV, usually 3300.0*/
    float t_resistance;             /**< Calculated thermistor resistance.*/
    uint32_t vout;                  /**< Voltage in mV of thermistor channel.*/                          
} thermistor_handle_t;

/**
 * @brief Initialice the thermistor driver.
 *
 * This function configure the ADC, and calibrate the reference voltage
 * to read the vout from resitance divider.
 *
 * @param   th  Pointer to store the driver information.
 * @param   channel ADC channel pin where the thermistor is connected.
 * @param   serial_resistante Value of the serial resistor connected to +3V.
 * @param   nominal_resistance Nominal resistance at 25 degrees Celsius of thermistor.
 * @param   nominal_temperature Nominal temperature of the thermistor, usually 25 degress Celsius.
 * @param   beta_val Beta coefficient of the thermistor.
 * @param   vsource Voltage to which the serial resistance is connected in mV, usually 3300.0.
 *
 * @return
 *      - ESP_OK: Initialization OK.
 */
uint32_t thermistor_init(thermistor_handle_t* th,
                         adc_channel_t channel, float serie_resistance, 
                         float nominal_resistance, float nominal_temperature, 
                         float beta_val, float vsource);

/**
 * @brief Read the vout of the resistance divider in mV.
 *
 * This function reads the value from the ADC and converts it to voltage in mV, 
 * using the calibration information from the reference.
 *
 * @param   th  Pointer of the driver information.
 *
 * @return
 *      - Vout in mV.
 */
uint32_t thermistor_read_vout(thermistor_handle_t* th);

/**
 * @brief Converts the output voltage of the divider to degrees Celsius.
 *
 * To linearize the thermistor output use the simplified steniarth equation.
 *
 * @param   th  Pointer of the driver information.
 * @param   vout Output voltage of the resistive divider in mV.
 *
 * @return
 *      - Temperature in degrees Celsius.
 */
float thermistor_vout_to_celcius(thermistor_handle_t* th, uint32_t vout);

/**
 * @brief Get temperature in degrees Celsius from the thermistor.
 *
 * This function calls thermistor_read_vout to read the voltage from the resistive 
 * divider and thermistor_vout_to_temp to convert it to degrees centigrade.
 * 
 * @param   th  Pointer of the driver information.
 *
 * @return
 *      - Temperature in degrees Celsius.
 */
float thermistor_get_celcius(thermistor_handle_t* th);

#ifdef __cplusplus
}
#endif

#endif /* __THERMISTOR_H__ */