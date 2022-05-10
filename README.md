# GCode
Arduino library for GCode parsing and optimized CNC controlling

This library can parse basic GCode (most important G/M codes supported) from any text based source.
The implementation uses some modified Bresenham algorithm to support fast realtime step calculations for example on my Atmega168 based stepper driven 2.5D CNC machine.

Some supported features: 
- basic movements (line, circle)
- relative/abs mode
- inch/mm mode support
- acceleration/deceleration

Incomplete features:
- Hardware reated functions (motor start/stop, toolchange, etc) can be handled by user interaction

Tested with Eagle's PCBGCode extension: https://groups.io/g/pcbgcode
