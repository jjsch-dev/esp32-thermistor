# ESP32 Thermistor Example

This is an example of using the component [esp32-thermistor] (https://github.com/) to measure the temperature using a [thermistor] (https://www.murata.com/~/media/Webrenewal/Support /library/catalog/products/thermistor/ntc/r44e.ashx?la=en-us) connected to an ADC channel of ESP32-C3.

![alt text](images/NXRT15WF104FA1B.png)

Although the implementation has been demonstrated with the NXRT15WF104FA1B of Murata, knowing the beta coefficient that the manufacturer publishes, can be used any other available in the market with the same component, an examples could be those using 3D printers for bed or the hot-end.

## Circuit

The thermistor is part of a resistive divider, where one of its ends is connected to GND and the other to the digital analog channel of ESP32-C3 plus the series resistor whose end is connected to 3.3 V.

![alt text](images/Schematic.png)

It is important to bear in mind that the resistance of the series has to have a tolerance of 1% or better, and if the footprint allows the dissipated power to be better than an order of magnitude of the maximum current that crosses it so that the stability is high.

Espressif [recomend](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc.html) a 0.1uF capacitor to the ADC input to minimize noise.

![alt text](images/adc-noise-graph.png)

## Prototype 

This component is the thermostat of an IOT project of the ceiling fan, so I started testing it on the next development board.

![alt text](images/pcb_proto_1.png)

## Behavior analysis

To evaluate the performance of a thermistor that is connected to an analog digital converter, (in addition to the quality and precision of it), many things can alter the result, for example the stability of the power source, the Ripple of VDD 3.3V , the resolution of the series resistance, but especially the linearity of the ADC converter that has implemented Espressif in ESP32.
I must say that the quality of the ESP32-C3 has surprised me, after using the characterization function of the ADC, the mV measurement is extremely accurate for a processor of this price. 

In the following images you can compare the measurement made with the oscilloscope at the analog channel input of the ESP32-C3 and the monitor output where the measured thermistor temperature is logged, as you can see the difference is a few mV.

![alt text](images/TEK_930mv.png)
![alt text](images/monitor_935mv.png)

AC noise with wifi / bluetooth off is better than 15mV.

![alt text](images/TEK_noise.png)

