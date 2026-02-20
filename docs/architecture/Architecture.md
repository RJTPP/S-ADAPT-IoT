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
| Encoder SW | User input | PA9 | `SW_Pin` |
| Extra button | Page switch input | PB0 | `BUTTON_Pin` |

## Firmware Modules
| Module | Main Files | Responsibility |
|---|---|---|
| App orchestration | `S-ADAPT/Core/Src/app.c` | Main runtime step, sensor read cadence, RGB state evaluation, and diagnostics |
| Ultrasonic driver | `S-ADAPT/Core/Src/ultrasonic.c` | TRIG pulse, TIM2 input capture, timeout/noise handling, distance conversion |
| Display driver facade | `S-ADAPT/Core/Src/display.c` | OLED init and page rendering calls |
| Status LED control | `S-ADAPT/Core/Src/status_led.c` | State-to-color mapping, polarity abstraction, and non-blocking fatal blink |
| Platform init | `S-ADAPT/Core/Src/main.c` | CubeMX init, app startup, infinite loop |

## Runtime Data Flow
```mermaid
flowchart TD
    A["Boot / HAL Init"] --> B["Peripheral Init (GPIO, TIM, I2C, ADC, UART)"]
    B --> C["app_init()"]
    C --> C1["status_led_init + ultrasonic_init + display_init"]
    C1 --> D["Loop: app_step()"]
    D --> E["status_led_tick(HAL_GetTick())"]
    D --> F["Every 100 ms: ultrasonic_read_distance_cm()"]
    F --> G["Update cached presence/distance on valid reads"]
    G --> H["Evaluate RGB state priority"]
    H --> I["Optional APP_RGB_TEST_MODE override"]
    I --> J["status_led_set_state + status_led_tick"]
    J --> K["GPIO outputs (RGB)"]
    G --> L["display_show_distance_cm()"]
    L --> M["OLED update (I2C)"]
```

## Timing Model (Current)
| Activity | Current cadence |
|---|---|
| Main loop | Free-running (`while(1)` with `app_step()`, no loop `HAL_Delay`) |
| App control tick | 100 ms (`APP_LOOP_TICK_MS`) |
| Ultrasonic measurement | Once per app control tick |
| OLED distance update | Once per app control tick when display init succeeds |
| Fatal RGB blink toggle | 250 ms default (`STATUS_LED_FAULT_BLINK_DEFAULT_MS`) |

## RGB Status Behavior (Current)
- Primary path is state-based RGB (`status_led_set_state`), not distance-threshold mapping.
- State colors:
- `BOOT_SETUP` -> Purple
- `AUTO` -> Blue
- `OFFSET_POSITIVE` -> Green
- `NO_USER` -> Red
- `FAULT_FATAL` -> blinking Red
- Compatibility wrappers (`status_led_set_for_distance`, `status_led_blink_error`) remain for transition only.
- Temporary compile-time test mode (`APP_RGB_TEST_MODE`) cycles `AUTO -> OFFSET_POSITIVE -> NO_USER -> BOOT_SETUP` every 1000 ms.

## Planned Direction
- Keep module boundaries stable.
- Disable/remove temporary RGB test mode after board validation.
- Complete full business logic layer: `AUTO + manual_offset`, light ON/OFF toggling, LDR-driven PWM output, and page switching.
- Keep hardware drivers separate from policy decisions.
