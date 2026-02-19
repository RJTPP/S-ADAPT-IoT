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
