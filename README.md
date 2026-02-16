# S-ADAPT

S-ADAPT is an STM32 embedded project for adaptive desk lighting.  
The system is intended to adjust lamp behavior based on ambient light and user presence.

This repository currently contains firmware and project files for STM32CubeIDE.

## Project Goal



- Detect user presence with an ultrasonic sensor.
- Measure ambient light with an LDR (ADC).
- Adjust LED brightness with PWM in real time.
- Show sensor/system status on an I2C OLED.
- Turn lights off automatically when no user is detected.

## Current Implementation Status

Implemented in firmware now:

- HC-SR04 distance measurement using GPIO + timer timing.
- SSD1306 OLED output over I2C (distance display).


Planned / not yet complete:

- LDR ADC sampling path.
- Moving average and median filtering pipeline from proposal.
- Full adaptive PWM brightness control and mode/state logic.

## Hardware (Planned/Used)

- STM32 Nucleo (L432 class project setup)
- HC-SR04 ultrasonic sensor
- I2C OLED (SSD1306)
- LDR + voltage divider
- LED driver stage (transistor/MOSFET) for PWM output

