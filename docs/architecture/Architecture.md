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
| RGB Status R | Status indication | PB4 | `LED_Status_R_Pin` |
| RGB Status G | Status indication | PB5 | `LED_Status_G_Pin` |
| RGB Status B | Status indication | PA11 | `LED_Status_B_Pin` |
| Encoder CLK | User input | PB1 | `ENCODER_CLK_EXTI1_Pin` |
| Encoder DT | User input | PA10 | `ENCODER_DT_EXTI10_Pin` |
| Encoder SW | User input | PA9 | `ENCODER_PRESS_Pin` |
| Extra button | Page switch input | PB0 | `BUTTON_Pin` |

## Firmware Modules
| Module | Main Files | Responsibility |
|---|---|---|
| Switch input debounce | `S-ADAPT/Core/Src/input/switch_input.c` | Poll `BUTTON`/`SW2`, debounce transitions, queue switch events |
| Main LED PWM driver | `S-ADAPT/Core/Src/bsp/main_led.c` | TIM1 CH1 PWM output control (`0..100%`) for MOSFET-driven lamp |
| Ultrasonic driver | `S-ADAPT/Core/Src/sensors/ultrasonic.c` | TRIG pulse, TIM2 input capture, timeout/noise handling, distance conversion |
| Display driver facade | `S-ADAPT/Core/Src/bsp/display.c` | OLED init and rendering calls via `ssd1306.c` |
| Status LED control | `S-ADAPT/Core/Src/bsp/status_led.c` | RGB indication and error blink support |
| Platform runtime entry | `S-ADAPT/Core/Src/main.c` | CubeMX/HAL init and app handoff (`app_init`, `app_step`) |
| App orchestration | `S-ADAPT/Core/Src/app/*.c` | Runtime state, events, sensing, control loop, RGB policy, OLED pages/overlay, diagnostics |

## Runtime Data Flow
```mermaid
flowchart TD
    A["Boot / HAL Init"] --> B["Peripheral Init (GPIO, TIM, I2C, ADC, UART)"]
    B --> C["main.c: app_init(hw)"]
    C --> D["main.c loop: app_step()"]
    D --> E["Input ticks/events (switch + encoder)"]
    E --> F["Click handling (single/double) + offset updates"]
    F --> G{"50 ms elapsed?"}
    G -- "Yes" --> H["LDR sample + MA8 filter update"]
    G -- "No" --> I["Reuse cached LDR value"]
    H --> J{"100 ms elapsed?"}
    I --> J
    J -- "Yes" --> K["Ultrasonic sample + median3 + presence engine"]
    J -- "No" --> L["Reuse cached distance/presence"]
    K --> M{"33 ms control tick?"}
    L --> M
    M -- "Yes" --> N["AUTO+offset -> hysteresis -> ramp -> main LED PWM"]
    M -- "No" --> O["Skip control update"]
    N --> P["RGB state eval + status_led update/tick"]
    O --> P
    P --> Q["OLED render (dirty/event driven, <=15 FPS, 1 s fallback)"]
    Q --> R["1 s summary UART log"]
```

## Timing Model (Current)
| Activity | Current cadence |
|---|---|
| Main loop pacing | `HAL_Delay(1)` |
| Control update tick | 33 ms (`control_tick_ms`) |
| Switch sampling | 10 ms (`SWITCH_SAMPLE_PERIOD_MS`) |
| Switch debounce confirmation | 20 ms (`SWITCH_DEBOUNCE_TICKS` x sample period) |
| LDR sampling | 50 ms (`ldr_sample_ms`) |
| Ultrasonic measurement | 100 ms (`us_sample_ms`) |
| Presence away timeout (current build profile) | 5000 ms (`APP_PRESENCE_DEBUG_TIMERS=1`) |
| Presence stale timeout (current build profile) | 15000 ms (`APP_PRESENCE_DEBUG_TIMERS=1`) |
| Pre-off dim duration (current build profile) | 5000 ms (`APP_PRESENCE_DEBUG_TIMERS=1`) |
| OLED redraw (dirty) | Up to ~15 FPS (`ui_min_redraw_ms = 66 ms`) |
| OLED refresh fallback | 1000 ms (`ui_refresh_ms`) |
| OLED overlay timeout | 1200 ms from last encoder step, plus post-reach hold (~750 ms) |
| UART summary log | 1000 ms (`log_ms`) |

Note: if `APP_PRESENCE_DEBUG_TIMERS` is set to `0`, the production timing profile becomes away `30000 ms`, stale `120000 ms`, and pre-off dim `10000 ms`.

## Known Bring-Up Note
- A branch-level bring-up issue was observed with RGB on `PA5/PA6/PA7`: enabling those channels caused OLED I2C timeout/busy (`HAL_I2C` error `0x20`).
- Remapping RGB to `PB4/PB5/PA11` resolved OLED stability in the current hardware setup.

## Planned Direction
- Keep module boundaries stable.
- Keep hardware drivers separate from policy decisions.
- Hardware bring-up and baseline business logic are integrated.
- Current focus is release stabilization: calibration, regression, and UX tuning.
- Keep `app` decomposition clean as features evolve.
