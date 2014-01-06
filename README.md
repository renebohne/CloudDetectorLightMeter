CloudDetectorLightMeter
=======================

An open-source cloud detector and light meter. It detects clouds and reads the light pollution of the sky.

Required Arduino Libraries:
OneWire: http://www.pjrc.com/teensy/td_libs_OneWire.html
FreqMeasure: http://www.pjrc.com/teensy/td_libs_FreqMeasure.html
i2master by Peter Fleury: http://jump.to/fleury or http://homepage.hispeed.ch/peterfleury/avr-software.html

How does a Cloud Detector work?
http://www.kwos.org/clouds_detector.htm

Arduino Pin Mapping:
D8: TSL237
D9: DS18B20 
A4,A5: I2C for MLX90614

SD Card (SPI):
D4,D10,D11,D12,D13

