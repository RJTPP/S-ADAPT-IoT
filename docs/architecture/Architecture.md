# S-ADAPT Architecture

## Purpose
This document defines the current firmware architecture and hardware-to-code mapping for S-ADAPT.

## Hardware Map
| Hardware | Role | MCU Signal | Firmware Symbol |
|---|---|---|---|
| HC-SR04 TRIG | Ultrasonic trigger output | PA0 | `TRIG_Pin` |
| HC-SR04 ECHO | Ultrasonic echo input capture | PA1 / TIM2_CH2 | `ECHO_TIM2_CH2_Pin` |
| OLED SDA | I2C data | PB7 | `OLED_I2C_SDA_Pin` |
| OLED SCL | I2C clock | PB6 | `OLED_I2C_SCL_Pin` |
| Main LED PWM | Lamp brightness control | PA8 / TIM1_CH1 | `Main_LED_TIM1_CH1_Pin` |
| RGB Status R | Status indication | PA5 | `LED_Status_R_Pin` |
| RGB Status G | Status indication | PA6 | `LED_Status_G_Pin` |
| RGB Status B | Status indication | PA7 | `LED_Status_B_Pin` |
| Encoder CLK | User input | PB1 | `ENCODER_CLK_EXTI1_Pin` |
| Encoder DT | User input | PA10 | `ENCODER_DT_EXTI10_Pin` |
| Encoder SW | User input | PA9 | `ENCODER_PRESS_Pin` |
| Extra button | Page switch input | PB0 | `BUTTON_Pin` |

## Firmware Modules
| Module | Main Files | Responsibility |
|---|---|---|
| Switch input debounce | `S-ADAPT/Core/Src/switch_input.c` | Poll `BUTTON`/`SW2`, debounce transitions, queue switch events |
| Ultrasonic driver | `S-ADAPT/Core/Src/ultrasonic.c` | TRIG pulse, TIM2 input capture, timeout/noise handling, distance conversion |
| Display driver facade | `S-ADAPT/Core/Src/display.c` | OLED init and rendering calls (available, not in current runtime loop) |
| Status LED control | `S-ADAPT/Core/Src/status_led.c` | RGB indication and error blink support (available) |
| Platform runtime | `S-ADAPT/Core/Src/main.c` | CubeMX init, runtime scheduling, switch event logging, ultrasonic logging |
| App orchestration (future) | `S-ADAPT/Core/Src/app.c` | Reserved for higher-level business logic orchestration |

## Runtime Data Flow
```mermaid
flowchart TD
    A["Boot / HAL Init"] --> B["Peripheral Init (GPIO, TIM, I2C, ADC, UART)"]
    B --> C["ultrasonic_init() + switch_input_init()"]
    C --> D["Loop in main.c"]
    D --> E["switch_input_tick(now_ms)"]
    E --> F["switch_input_pop_event()"]
    F --> G["UART: switch pressed/released transitions"]
    D --> H{"100 ms elapsed?"}
    H -- "Yes" --> I["ultrasonic_read_echo_us()"]
    I --> J["Distance conversion + status read"]
    J --> K["UART: ultrasonic diagnostic line"]
    H -- "No" --> L["Idle"]
    G --> L
    K --> L
```

## Timing Model (Current)
| Activity | Current cadence |
|---|---|
| Main loop pacing | `HAL_Delay(1)` |
| Switch sampling | 10 ms (`SWITCH_SAMPLE_PERIOD_MS`) |
| Switch debounce confirmation | 30 ms (`SWITCH_DEBOUNCE_TICKS` x sample period) |
| Ultrasonic measurement | 100 ms (`US_SAMPLE_PERIOD_MS`) |
| UART switch logs | On debounced transitions only |
| UART ultrasonic logs | Once per ultrasonic tick |

## Planned Direction
- Keep module boundaries stable.
- Keep hardware drivers separate from policy decisions.
- Add LDR and PWM hardware bring-up modules to the same scheduling model.
- Move scheduling/orchestration from `main.c` to `app.c` when business logic integration starts.
