# Project Proposal: S-ADAPT

## 1. Introduction

### 1.1 Background and Significance

Reading books in inadequate or excessive lighting conditions is detrimental to eye health. Furthermore, forgetting to turn off lights when leaving a desk results in unnecessary energy consumption. This project aims to create an intelligent lighting control system that automatically adjusts brightness to suit the environment and operates only when a user is present.

### 1.2 Primary Objectives

- To develop an automatic light switching system based on user distance detection.
- To implement a real-time brightness adjustment system that responds to ambient light levels.

### 1.3 Project Scope

**Input:**

- **LDR Sensor (Analog):** Periodic sampling of ambient light intensity.
- **Ultrasonic Sensor (Digital):** Periodic sampling of distance to verify user presence.

**Processing:**

- **Moving Average Filter:** Applied to LDR data to reduce noise and prevent rapid flickering of the light.
- **Median Filter:** Applied to Ultrasonic data to filter out zero values or abnormal distance spikes.
- **PWM (Pulse Width Modulation):** Controls LED brightness inversely proportional to detected ambient light.

**Decision Logic:**

- **Condition Active:** If Distance < Threshold (user detected) and light is low, turn on the light at the calculated brightness level.
- **Condition Standby:** If Distance > Threshold (no user) for a set duration, automatically turn off the light.

**Output:**

- **OLED Display:** Shows real-time light intensity (%) and system status (Active/Standby).
- **Indicator LED:** On-board LED used to show system operational status and sensor activity.

## 2. Proposed Method

### 2.1 Equipment

- **Microcontroller:** STM32 Board (Nucleo/Discovery)
- **Sensors:**
  - LDR Photoresistor (Analog)
  - Ultrasonic Sensor HC-SR04 (Digital)
- **Actuator/Display:**
  - LED Module (PWM-capable)
  - OLED Display (I2C)
- **Software:**
  - STM32CubeIDE
  - STM32 HAL Library

### 2.2 Circuit Design (วงจรที่ต้องออกแบบเพิ่ม)

- **Voltage Divider:** For the LDR sensor to adapt the voltage range for the ADC.
- **LED Driver:** Using a Transistor or MOSFET to handle power requirements and PWM signals.

### 2.3 Operational Steps (ขั้นตอนการทำงาน)

1. **Sensor Acquisition:** Sample LDR via ADC and capture Ultrasonic echo width via GPIO Timer Input Capture every 100 ms.
2. **Processing:** Apply Moving Average filtering to LDR values. Convert Ultrasonic time data to centimeters and apply a Median Filter.
3. **Decision Logic:** Check if Distance < Threshold. If yes, reset the timeout timer. If no presence is detected for 30 seconds, switch to OFF status.
4. **Hysteresis:** Utilize a software Schmitt-trigger logic for brightness updates. PWM values change only when light levels cross a specific threshold to prevent oscillation.
5. **Display/Indicator:** Update the OLED screen via I2C with light percentage, distance, and mode.

## 3. Expected Outcomes (ผลลัพธ์ที่คาดหวัง)

- The system can measure light and distance accurately with stable readings.
- The LED turns on/off automatically based on presence and predefined timers.
- Brightness transitions are smooth and flicker-free.
- Real-time status is correctly displayed on the OLED screen.

## 4. Acceptance Criteria (เกณฑ์ความสำเร็จ)

- **Periodic Sampling:** LDR (50 ms $\pm 10$%), Ultrasonic (100 ms $\pm 10$%).
- **Data Quality:** Filtered data must appear smooth (no spikes) when viewed via Serial Plotter or STM32CubeMonitor.
- **Visual Stability:** PWM output must remain steady even if ambient light fluctuates slightly.

## 5. Minimum Requirement Checklist (Checklist ข้อกำหนดขั้นต่ำ)

### 5.1 Sensor Input

- [x] **Analog (ADC):** LDR Photoresistor.
- [x] **Digital (GPIO/I2C/SPI):** Ultrasonic Sensor HC-SR04.
- [x] **Periodic Sampling:** Continuous reading of light intensity and distance.

### 5.2 Processing Logic on STM32

- [x] **Data Processing:** Noise reduction for ADC (LDR) and unit conversion (Ultrasonic).
- [x] **Decision Logic:** Distance-based thresholding, timer management, and hysteresis for brightness.
- [x] **Data Validation:** Checking for distance spikes/jumps to discard noise.

### 5.3 Display/Indicator

- [x] **Sensor Display:** Light intensity and distance.
- [x] **System Status:** Operation mode (Auto, Manual, Off).
- [x] **Event Indicator:** On-board LED blinking for heartbeat/status.
