# S-ADAPT Planning Checklist

Source references:
- `docs/references/Project Proposal.md`
- `docs/architecture/Schematic_S-ADAPT.png`
- Feature summary and logic notes from latest planning input.
- Firmware module layout refactor:
  - `S-ADAPT/Core/Inc/{app,bsp,sensors,input,support}`
  - `S-ADAPT/Core/Src/{app,bsp,sensors,input,support}`
  - Qualified includes from `Core/Inc` root (for example `#include "sensors/ultrasonic.h"`).

## 1. Hardware Handling Code

### Current hardware status
- [x] RGB, LDR, switch/button, encoder, main LED PWM, and OLED bring-up are validated on target board.
- [x] Pin/peripheral mapping in firmware matches current schematic wiring.
- [x] OLED I2C path is stable (`0x3C`), including runtime updates with other peripherals active.
- [x] HC-SR04 trigger/echo measurement and timeout path are validated.
- [x] LDR ADC range/response is validated.
- [x] Main LED PWM path is validated (off to max range, stable switching under normal operation).
- [x] RGB state mapping and fault indication path are validated.
- [x] Encoder/button input behavior is validated (debounce + event semantics).
- [x] Runtime build/flash behavior is validated in normal development flow.

## 2. Business Logic

### Power-on defaults
- [x] `Mode = AUTO` (with `manual_offset = 0`)
- [x] `Light = OFF` on boot (PWM output forced to 0 until user action)
- [x] `OLED Page = 0`

### Sampling and filtering
- [x] Control loop tick at 33 ms (+/- 10%).
- [x] LDR sampling loop at 50 ms (+/- 10%), decoupled from control-tick cadence.
- [x] Ultrasonic sampling loop at 100 ms (+/- 10%).
- [x] Moving average filter for LDR.
- [x] Median/outlier filtering for ultrasonic.
- [x] Hysteresis (Schmitt-trigger style thresholds) for stable brightness updates.
- [x] Presence engine uses reference capture + timed away/stale detection.

### Presence and safety logic
- [x] Capture reference distance on each OFF->ON click.
- [x] Away detection: `distance > ref + 20 cm` for 5s in debug-timer profile (`APP_PRESENCE_DEBUG_TIMERS=1`) -> no-user candidate.
- [x] Stale/flat detection from low step-to-step movement (`abs(step) <= 1 cm`) for 15s in debug-timer profile (`APP_PRESENCE_DEBUG_TIMERS=1`) -> no-user candidate.
- [x] Pre-off dim phase: `min(current,15%)` for 5s in debug-timer profile (`APP_PRESENCE_DEBUG_TIMERS=1`) before no-user commit.
- [x] Recovery from away/no-user near reference (`ref + 10 cm`) with short confirm delay.
- [x] Recovery from flat/no-user on movement spike (`abs(step) >= 2 cm`).
- [x] If `user_present = FALSE`, force `brightness = 0`.

### AUTO + offset logic
- [x] AUTO mode:
- [x] Read LDR and compute target brightness (dark -> higher PWM, bright -> lower PWM).
- [x] Clamp output range (0% to 100% baseline).
- [x] Offset adjustment behavior:
- [x] Encoder CW increases manual brightness offset.
- [x] Encoder CCW decreases manual brightness offset.
- [x] Encoder rotation adjusts `manual_offset` only (not LDR sensitivity or curve factor).
- [x] Offset is applied on top of AUTO target brightness and clamped to valid PWM range.
- [x] Encoder single click toggles light ON/OFF.
- [x] Encoder double click resets manual offset to `0` (revert to pure AUTO brightness).

### Output update logic
- [x] Apply PWM output every control tick.
- [x] If light state is OFF, output ramps down to 0 with configured slew limit.
- [x] Apply smoothing/hysteresis/ramp to avoid flicker and abrupt jumps.
- [x] Output ramp limiter implemented (normal 1%, turn-on 3%, turn-off 5% per 33 ms tick).
- [x] Update RGB LED according to system state.

### Output behavior
- [x] OLED includes:
- [x] Brightness %
- [x] Current control profile (`AUTO + offset`) and light ON/OFF state
- [x] Distance
- [x] Ambient light value (raw and/or filtered)
- [x] Multiple OLED pages are implemented.
- [x] Extra button cycles pages: `page = (page + 1) % TOTAL_PAGES`.
- [x] Encoder rotation triggers temporary OLED offset overlay (e.g., for ~1200 ms).
- [x] Overlay timeout resets on each new encoder step; page returns after timeout + overlay animation completion/hold.
- [x] Status LED behavior is explicitly documented (kept for now).
- [x] Optional UART debug output for tuning thresholds and filters.

### Settings UI and persistence
- [x] Modal settings mode entered/exited by `BUTTON` long-press (`1000 ms`).
- [x] Encoder is reused for settings navigation/editing (`rotate`/`click`).
- [x] Draft-edit model is implemented (`Save` applies, `Exit` discards unsaved edits).
- [x] Core presence settings are configurable on-device:
- [x] away mode enable
- [x] flat mode enable
- [x] away timeout
- [x] stale timeout
- [x] pre-off dim duration
- [x] return band
- [x] Internal flash persistence module is implemented (append-only record with `magic/version/seq/crc`).
- [x] Linker flash region is reduced to reserve one NVM page (`2 KB`).
- [x] On-board save/reboot/restore behavior is validated and recorded.
- [x] Corrupt-record fallback behavior is validated on target.

Note: production timing profile (`APP_PRESENCE_DEBUG_TIMERS=0`) is away `30s`, stale `120s`, pre-off dim `10s`.

### Acceptance checks
- [x] Presence + low light test passes.
- [x] No presence timeout-off test passes.
- [x] Small ambient fluctuations do not cause output oscillation.
- [x] Single click toggles lamp state reliably.
- [x] Double click resets manual offset to AUTO baseline.
- [x] Encoder rotation adjusts brightness offset smoothly.
- [x] Record test notes for PR.
