# S-ADAPT Business Logic

## Target Feature Behavior
- Adaptive lamp control using ambient light (LDR) and user presence (HC-SR04).
- Control model: `AUTO + manual_offset` (single mode).
- OLED multi-page status display.
- RGB LED indicates system state.

## Current Implementation Snapshot (Driver-First Phase)
- Runtime owner is currently `main.c` (hardware bring-up mode), not `app.c`.
- Hardware drivers are integrated: LDR, ultrasonic, switch, encoder, RGB status LED, OLED facade, main LED PWM.
- Main LED is currently validated via deterministic debug duty sweep (`0/25/50/75/100%` every 1 s).
- OLED debug heartbeat/update path is active when display init succeeds.
- RGB debug state cycle is active at 1 s cadence for hardware verification.

## Power-On Defaults (Current)
- `Mode = AUTO`
- `manual_offset = 0`
- `Light = OFF` (represented by `light_enabled = 0`)
- `distance_cm` initialized to error value (`999`)

## Core State Variables (Current Subset)
- `mode`: `AUTO`
- `light_enabled`: boolean ON/OFF
- `manual_offset`: signed brightness offset (e.g. `-30..+30`)
- `last_valid_presence`: boolean from distance threshold (`80 cm`)
- `last_valid_distance_cm`: last valid ultrasonic value
- `fatal_fault`: fatal status flag for RGB blink override

## Main Control Flow (Current)
```mermaid
flowchart TD
    A["app_step()"] --> B["status_led_tick(now_ms)"]
    B --> C{"100 ms elapsed?"}
    C -- "No" --> D["Return"]
    C -- "Yes" --> E["Read ultrasonic + status"]
    E --> F{"Valid read?"}
    F -- "Yes" --> G["Update last_valid_distance and last_valid_presence"]
    F -- "No" --> H["Keep previous cached presence/distance"]
    G --> I["Optional OLED distance update"]
    H --> I
    I --> J["Evaluate runtime RGB state priority"]
    J --> K["Optional test override cycle"]
    K --> L["status_led_set_state()"]
    L --> M["status_led_tick(now_ms)"]
```

## Presence Logic (Current)
- Threshold example: `80 cm` (calibrate on real hardware).
- Presence cache only changes when ultrasonic returns `ULTRASONIC_STATUS_OK`.
- On transient ultrasonic failure, cached presence is held (no RGB flicker from invalid samples).

## RGB Mapping (Current)
- `BOOT_SETUP` -> Purple
- `AUTO` -> Blue
- `OFFSET_POSITIVE` -> Green
- `NO_USER` -> Red
- `FAULT_FATAL` -> blinking Red

## RGB Priority (Current)
1. `FAULT_FATAL`
2. `BOOT_SETUP` for first `1000 ms` after init
3. `NO_USER` when `light_enabled == 1` and `last_valid_presence == 0`
4. `OFFSET_POSITIVE` when `light_enabled == 1` and `manual_offset > 0`
5. `AUTO`

## Temporary RGB Validation Mode
- Compile-time flag: `APP_RGB_TEST_MODE`.
- Current setting: enabled for board validation.
- Behavior when enabled:
- every `1000 ms`, override runtime color with cycle:
- `AUTO -> OFFSET_POSITIVE -> NO_USER -> BOOT_SETUP -> repeat`
- `FAULT_FATAL` still has highest priority and is not overridden.

## Legacy Compatibility Status
- `status_led_set_for_distance()` remains as a deprecated wrapper for legacy callers.
- `status_led_blink_error()` remains as a shim and routes into non-blocking fatal blink handling.

## Remaining Work To Reach Full Target Logic
- Baseline business logic first:
- integrate `light_enabled`, presence gate, `AUTO + manual_offset`, and PWM output from control decision.
- keep deterministic, easy-to-debug tick schedule while integrating.
- Then add stabilization utils:
- moving average for LDR.
- median/outlier handling for ultrasonic.
- hysteresis (Schmitt-trigger style thresholds) for state transitions and no-user flicker control.
- Then implement UX behaviors:
- encoder single click/double click policy, rotation offset behavior, OLED page/overlay behavior.

## Implementation Order (Locked for Next Phase)
1. Baseline control loop (no advanced filtering): correct behavior first.
2. Filtering/hysteresis utilities: improve stability without changing intent.
3. UI polish and tuning: OLED page model, debug reduction, threshold tuning on board.

## Target State Diagram (Planned)
```mermaid
stateDiagram-v2
    [*] --> AUTO_OFF
    AUTO_OFF --> AUTO_ON: Single click (light ON)
    AUTO_ON --> AUTO_OFF: Single click (light OFF)
    AUTO_ON --> STANDBY: No user (timeout or immediate rule)
    AUTO_OFF --> STANDBY: No user (optional policy)
    STANDBY --> AUTO_OFF: User returns - wait for single click
```
