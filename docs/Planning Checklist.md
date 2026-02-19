# S-ADAPT Planning Checklist

Source references:
- `docs/Project Proposal.md`
- `docs/Schematic_S-ADAPT.png`

## 1. Hardware Handling Code

### Pin and peripheral setup
- [ ] Verify mapping in code matches schematic:
- [ ] `PA0` = HC-SR04 `TRIG` (GPIO output)
- [ ] `PA1` = HC-SR04 `ECHO` (`TIM2_CH2` input capture)
- [ ] `PB6/PB7` = OLED I2C (`SCL/SDA`)
- [ ] `PA8` = PWM output for main LED driver (`TIM1_CH1`)
- [ ] Confirm LDR analog pin and ADC channel mapping are consistent.

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
- [ ] RGB status LED module:
- [ ] Distance-status indication behavior is correct (kept for now).
- [ ] Error blink path works on forced init failure.
- [ ] Encoder module (`CLK/DT/SW`) inputs:
- [ ] Pin reads/interrupts are stable and debounced as needed.
- [ ] Logic safely ignores encoder inputs when unused.
- [ ] Push switch (`SW2`) input:
- [ ] Press/release logic is stable (no false toggles).
- [ ] +5V / +3.3V rails and decoupling:
- [ ] Voltage levels are within expected range under load.
- [ ] Power-up does not cause OLED/sensor brownout or random resets.

### Driver/module readiness
- [ ] Keep modules clean and single-purpose (`ultrasonic`, `display`, `status_led`, `app`).
- [ ] Ensure ultrasonic capture handles timeout and noisy/overcapture cases.
- [ ] Ensure OLED init and updates are stable via I2C (no blocking failure loops except fatal init).
- [ ] Keep distance LED indicator behavior for now (as currently implemented).

### Hardware-level validation
- [ ] Build `Debug` successfully.
- [ ] Verify ultrasonic distance updates on target board.
- [ ] Verify OLED text updates on target board.
- [ ] Verify PWM output to LED driver hardware path.

## 2. Business Logic

### Sampling and filtering
- [ ] LDR sampling loop at 50 ms (+/- 10%).
- [ ] Ultrasonic sampling loop at 100 ms (+/- 10%).
- [ ] Moving average filter for LDR.
- [ ] Median/outlier filtering for ultrasonic.

### Decision logic
- [ ] Define and document presence threshold (cm).
- [ ] If user present and ambient is low: light on with computed PWM brightness.
- [ ] If user absent for 30 s: transition to standby/off.
- [ ] Add hysteresis for brightness updates to prevent flicker.

### Output behavior
- [ ] OLED shows distance, light percentage, and mode (`Active`/`Standby`).
- [ ] Status LED behavior is explicitly documented (kept for now).
- [ ] Optional UART debug output for tuning thresholds and filters.

### Acceptance checks
- [ ] Presence + low light test passes.
- [ ] No presence timeout-off test passes.
- [ ] Small ambient fluctuations do not cause output oscillation.
- [ ] Record test notes for PR.
