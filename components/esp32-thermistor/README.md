# esp32-thermistor

This is an example of using the component [ESP32-THERMISTOR] (https://github.com/) to measure the temperature using a [thermistor] (https://www.murata.com/~/media/Webrenewal/Support /library/catalog/products/thermistor/ntc/r44e.ashx?la=en-us) connected to an ADC channel of ESP32-C3.

![alt text](images/NXRT15WF104FA1B.png)

Although the implementation has been demonstrated with the NXRT15WF104FA1B of Murata, knowing the beta coefficient that the manufacturer publishes, can be used any other available in the market with the same component, an examples could be those using 3D printers for bed or the hot-end.

## Circuit

The thermistor is part of a resistive divider, where one of its ends is connected to GND and the other to the digital analog channel of ESP32-C3 plus the series resistor whose end is connected to 3.3 V.

![alt text](images/Schematic.png)

It is important to bear in mind that the resistance of the series has to have a tolerance of 1% or better, and if the footprint allows the dissipated power to be better than an order of magnitude of the maximum current that crosses it so that the stability is high.

Espressif [recomend](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc.html) a 0.1uF capacitor to the ADC input to minimize noise.

![alt text](images/adc-noise-graph.png)

