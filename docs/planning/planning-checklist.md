# S-ADAPT Planning Checklist

Source references:
- `docs/references/Project Proposal.md`
- `docs/references/Schematic_S-ADAPT.png`
- Feature summary and logic notes from latest planning input.
- Firmware module layout refactor:
  - `S-ADAPT/Core/Inc/{app,bsp,sensors,input,support}`
  - `S-ADAPT/Core/Src/{app,bsp,sensors,input,support}`
  - Qualified includes from `Core/Inc` root (for example `#include "sensors/ultrasonic.h"`).

## 1. Hardware Handling Code

### Current bring-up snapshot (driver-first)
- [x] RGB status LED state mapping is implemented and board-verified.
- [x] LDR raw ADC driver is integrated and sampled in runtime loop.
- [x] Switch input module is integrated with software debounce.
- [x] Encoder input module is integrated (EXTI-based CLK path).
- [x] Main LED PWM driver module is integrated (`TIM1_CH1`) with 1 s duty sweep debug.
- [x] OLED init/debug path is available and can run in OLED-only debug mode.

### Pin and peripheral setup
- [ ] Verify mapping in code matches schematic:
- [ ] `PA0` = HC-SR04 `TRIG` (GPIO output)
- [ ] `PA1` = HC-SR04 `ECHO` (`TIM2_CH2` input capture)
- [ ] `PB6/PB7` = OLED I2C (`SCL/SDA`)
- [ ] `PA8` = PWM output for main LED driver (`TIM1_CH1`)
- [ ] RGB status LED pins are mapped and tested:
- [ ] `LED_Status_R` = `PB4`
- [ ] `LED_Status_G` = `PB5`
- [ ] `LED_Status_B` = `PA11`
- [ ] Confirm LDR analog pin and ADC channel mapping are consistent.
- [ ] Verify encoder and button pin mapping for:
- [ ] Encoder `CLK`, `DT`, `SW`
- [ ] Extra button for OLED page switching

### Per-hardware checklist
- [ ] NUCLEO-L432KC core clocks/peripherals initialized as expected after boot.
- [ ] OLED module:
- [ ] I2C address responds (`0x3C` expected).
- [ ] Text render/update works without bus lockups.
- [ ] Validate OLED remains stable while RGB channels are active.
- [ ] If OLED fails with `HAL_I2C` timeout/busy (`err=0x20`), isolate pin/wiring interactions and retest RGB mapping.
- [ ] HC-SR04 module:
- [ ] `TRIG` pulse width is valid (~10 us).
- [ ] `ECHO` capture timing is stable at near and far targets.
- [ ] Timeout path works when no echo is returned.
- [ ] LDR + divider network:
- [ ] ADC raw values move correctly with light changes.
- [ ] Scaled light percentage range is reasonable and monotonic.
- [ ] Main LED module + MOSFET driver:
- [x] PWM duty changes match commanded brightness.
- [ ] LED fully turns off at 0% and reaches expected max brightness.
- [ ] No visible flicker at normal operating brightness.
- [ ] MOSFET gate resistor and pull-down behavior validated (clean switching).
- [ ] RGB status LED module:
- [ ] State color mapping works:
- [ ] Blue = Auto mode
- [ ] Green = Manual mode
- [ ] Yellow = No user (while light ON)
- [ ] Red = Manual OFF
- [ ] Purple = Setup / special mode
- [ ] Confirm implemented state-based RGB mapping matches behavior spec (distance-only legacy mapping removed).
- [ ] Error blink path works on forced init failure.
- [ ] Confirm no OLED/I2C disturbance when RGB outputs toggle.
- [ ] Encoder module (`CLK/DT/SW`) inputs:
- [ ] Pin reads/interrupts are stable and debounced as needed.
- [ ] Logic safely ignores encoder inputs when unused.
- [ ] Push switch (`SW2`) input:
- [ ] Press/release logic is stable (no false toggles).
- [ ] +5V / +3.3V rails and decoupling:
- [ ] Voltage levels are within expected range under load.
- [ ] 470 uF and 0.1 uF decoupling behavior validated under LED load changes.
- [ ] Power-up does not cause OLED/sensor brownout or random resets.

### Driver/module readiness
- [ ] Keep modules clean and single-purpose (`ultrasonic`, `display`, `status_led`, `app`).
- [ ] Ensure ultrasonic capture handles timeout and noisy/overcapture cases.
- [ ] Ensure OLED init and updates are stable via I2C (no blocking failure loops except fatal init).
- [ ] Keep module includes namespaced by domain (`app/...`, `bsp/...`, `sensors/...`, `input/...`, `support/...`).
- [ ] Ensure no direct includes depend on legacy flat header paths.

### Hardware-level validation
- [ ] Run CubeIDE clean/rebuild after file moves to regenerate build metadata (`sources.mk`/`subdir.mk`).
- [ ] Build `Debug` successfully.
- [ ] Build `Release` successfully.
- [ ] Verify ultrasonic distance updates on target board.
- [ ] Verify OLED text updates on target board.
- [x] Verify PWM output to LED driver hardware path.

## 2. Business Logic

### Power-on defaults
- [x] `Mode = AUTO` (with `manual_offset = 0`)
- [x] `Light = OFF` on boot (PWM output forced to 0 until user action)
- [ ] `OLED Page = 0`

### Sampling and filtering
- [x] LDR sampling loop at 50 ms (+/- 10%).
- [x] Ultrasonic sampling loop at 100 ms (+/- 10%).
- [x] Moving average filter for LDR.
- [x] Median/outlier filtering for ultrasonic.
- [x] Hysteresis (Schmitt-trigger style thresholds) for stable brightness updates.
- [ ] Presence hysteresis (optional, deferred).

### Presence and safety logic
- [x] Define and document presence threshold (target: ~80 cm, tune on board).
- [x] `distance < threshold` -> `user_present = TRUE`
- [x] `distance >= threshold` -> `user_present = FALSE`
- [x] If `user_present = FALSE`, force `brightness = 0`.
- [ ] Optional no-presence timeout behavior is defined (immediate off vs delayed off).

### AUTO + offset logic
- [x] AUTO mode:
- [x] Read LDR and compute target brightness (dark -> higher PWM, bright -> lower PWM).
- [x] Clamp output range (0% to 100% baseline).
- [ ] Offset adjustment behavior:
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
- [x] Output ramp limiter implemented (2% per 50 ms tick).
- [x] Update RGB LED according to system state.

### Output behavior
- [ ] OLED includes:
- [ ] Brightness %
- [ ] Current control profile (`AUTO + offset`) and light ON/OFF state
- [ ] Distance
- [ ] Ambient light value (raw and/or filtered)
- [ ] Multiple OLED pages are implemented.
- [ ] Extra button cycles pages: `page = (page + 1) % TOTAL_PAGES`.
- [ ] Encoder rotation triggers temporary OLED offset overlay (e.g., for ~1200 ms).
- [ ] Overlay timeout resets on each new encoder step and returns to previous page when expired.
- [ ] Status LED behavior is explicitly documented (kept for now).
- [ ] Optional UART debug output for tuning thresholds and filters.
- [x] Optional UART debug output for tuning thresholds and filters.

### Acceptance checks
- [ ] Presence + low light test passes.
- [ ] No presence timeout-off test passes.
- [ ] Small ambient fluctuations do not cause output oscillation.
- [ ] Single click toggles lamp state reliably.
- [ ] Double click resets manual offset to AUTO baseline.
- [ ] Encoder rotation adjusts brightness offset smoothly.
- [ ] Record test notes for PR.
