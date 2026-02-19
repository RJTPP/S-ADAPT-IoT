# S-ADAPT Planning Checklist

Source references:
- `docs/references/Project Proposal.md`
- `docs/references/Schematic_S-ADAPT.png`
- Feature summary and logic notes from latest planning input.

## 1. Hardware Handling Code

### Pin and peripheral setup
- [ ] Verify mapping in code matches schematic:
- [ ] `PA0` = HC-SR04 `TRIG` (GPIO output)
- [ ] `PA1` = HC-SR04 `ECHO` (`TIM2_CH2` input capture)
- [ ] `PB6/PB7` = OLED I2C (`SCL/SDA`)
- [ ] `PA8` = PWM output for main LED driver (`TIM1_CH1`)
- [ ] Confirm LDR analog pin and ADC channel mapping are consistent.
- [ ] Verify encoder and button pin mapping for:
- [ ] Encoder `CLK`, `DT`, `SW`
- [ ] Extra button for OLED page switching

### Per-hardware checklist
- [ ] NUCLEO-L432KC core clocks/peripherals initialized as expected after boot.
- [ ] OLED module:
- [ ] I2C address responds (`0x3C` expected).
- [ ] Text render/update works without bus lockups.
- [ ] HC-SR04 module:
- [ ] `TRIG` pulse width is valid (~10 us).
- [ ] `ECHO` capture timing is stable at near and far targets.
- [ ] Timeout path works when no echo is returned.
- [ ] LDR + divider network:
- [ ] ADC raw values move correctly with light changes.
- [ ] Scaled light percentage range is reasonable and monotonic.
- [ ] Main LED module + MOSFET driver:
- [ ] PWM duty changes match commanded brightness.
- [ ] LED fully turns off at 0% and reaches expected max brightness.
- [ ] No visible flicker at normal operating brightness.
- [ ] MOSFET gate resistor and pull-down behavior validated (clean switching).
- [ ] RGB status LED module:
- [ ] State color mapping works:
- [ ] Blue = Auto mode
- [ ] Green = Manual mode
- [ ] Red = No user / standby
- [ ] Purple = Setup / special mode
- [ ] Transition plan from current distance-based indication to state-based indication is defined.
- [ ] Error blink path works on forced init failure.
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
- [ ] Keep current distance-based RGB behavior for now until state-based RGB mapping is implemented.

### Hardware-level validation
- [ ] Build `Debug` successfully.
- [ ] Verify ultrasonic distance updates on target board.
- [ ] Verify OLED text updates on target board.
- [ ] Verify PWM output to LED driver hardware path.

## 2. Business Logic

### Power-on defaults
- [ ] `Mode = AUTO` (with `manual_offset = 0`)
- [ ] `Light = OFF` on boot (PWM output forced to 0 until user action)
- [ ] `OLED Page = 0`

### Sampling and filtering
- [ ] LDR sampling loop at 50 ms (+/- 10%).
- [ ] Ultrasonic sampling loop at 100 ms (+/- 10%).
- [ ] Moving average filter for LDR.
- [ ] Median/outlier filtering for ultrasonic.

### Presence and safety logic
- [ ] Define and document presence threshold (target: ~80 cm, tune on board).
- [ ] `distance < threshold` -> `user_present = TRUE`
- [ ] `distance >= threshold` -> `user_present = FALSE`
- [ ] If `user_present = FALSE`, force `brightness = 0`.
- [ ] Optional no-presence timeout behavior is defined (immediate off vs delayed off).

### Mode logic
- [ ] Auto mode:
- [ ] Read filtered LDR and compute target brightness (dark -> higher PWM, bright -> lower PWM).
- [ ] Clamp output range (example: 20% to 100%).
- [ ] Manual adjustment behavior:
- [ ] Encoder CW increases manual brightness offset.
- [ ] Encoder CCW decreases manual brightness offset.
- [ ] Offset is applied on top of AUTO target brightness and clamped to valid PWM range.
- [ ] Encoder single click toggles light ON/OFF.
- [ ] Encoder double click resets manual offset to `0` (revert to pure AUTO brightness).

### Output update logic
- [ ] Apply PWM output every control tick.
- [ ] If light state is OFF, force output to 0 regardless of auto/offset calculation.
- [ ] Apply smoothing/hysteresis/ramp to avoid flicker and abrupt jumps.
- [ ] Update RGB LED according to system state.

### Output behavior
- [ ] OLED includes:
- [ ] Brightness %
- [ ] Current mode
- [ ] Distance
- [ ] Ambient light value (raw and/or filtered)
- [ ] Multiple OLED pages are implemented.
- [ ] Extra button cycles pages: `page = (page + 1) % TOTAL_PAGES`.
- [ ] Status LED behavior is explicitly documented (kept for now).
- [ ] Optional UART debug output for tuning thresholds and filters.

### Acceptance checks
- [ ] Presence + low light test passes.
- [ ] No presence timeout-off test passes.
- [ ] Small ambient fluctuations do not cause output oscillation.
- [ ] Single click toggles lamp state reliably.
- [ ] Double click resets manual offset to AUTO baseline.
- [ ] Encoder rotation adjusts brightness offset smoothly.
- [ ] Record test notes for PR.
