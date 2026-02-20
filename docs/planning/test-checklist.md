# S-ADAPT Test Checklist

## Preconditions
- [ ] Firmware builds in `Debug`.
- [ ] Board powered with stable `+5V` and `+3.3V`.
- [ ] OLED connected and visible.
- [ ] HC-SR04, LDR, and LED driver wiring verified against schematic.

## A. Hardware Handling Code Tests

### A1. Sensor and interface bring-up
- [ ] OLED initializes and prints boot text.
- [ ] Ultrasonic distance updates continuously.
- [ ] LDR ADC value changes with lighting condition.
- [ ] PWM output present on `TIM1_CH1` pin.

### A2. Module-level robustness
- [ ] Ultrasonic timeout path returns safe value when echo is missing.
- [ ] Overcapture/noise handling does not lock control loop.
- [ ] OLED update loop does not freeze I2C bus.
- [ ] Error blink path works if display init is intentionally forced to fail.

### A3. Electrical behavior
- [ ] MOSFET gate control is clean (no chatter at steady duty).
- [ ] LED can fully turn off and reach target max output.
- [ ] No reset or brownout during fast brightness changes.

## B. Business Logic Tests

### B1. Presence and mode behavior
- [ ] User near threshold -> `user_present = true`.
- [ ] User beyond threshold -> standby/off behavior triggers as specified.
- [ ] Auto mode computes brightness from LDR.
- [ ] Boot state keeps main light OFF until user single-click.
- [ ] Encoder single click toggles main light ON/OFF.
- [ ] Encoder rotation adjusts manual brightness offset (CW up / CCW down).
- [ ] Encoder rotation changes offset only (does not change sensitivity/factor parameters).
- [ ] Double click resets manual offset to `0` (AUTO baseline).
- [ ] LDR-based AUTO logic remains active after offset reset.
- [ ] Presence behavior remains baseline threshold logic (no hysteresis in this phase).

### B1.1 Stability filter behavior
- [ ] `ldr_filt` responds smoother than `ldr_raw` under small ambient changes.
- [ ] `dist_cm_filt` rejects one-sample ultrasonic spikes better than raw.
- [ ] Output hysteresis deadband works:
- [ ] target change `<5%` does not update hysteresis output.
- [ ] target change `>=5%` updates hysteresis output.
- [ ] Output ramp works:
- [ ] applied output changes by at most `2%` per `50 ms` control tick.
- [ ] Forced-off paths (manual OFF / no-user) ramp down smoothly to `0%`.

### B2. OLED behavior
- [ ] Page button cycles pages with wraparound logic.
- [ ] Page 0 shows AUTO + offset + brightness + light ON/OFF state.
- [ ] Page 1 shows LDR info.
- [ ] Page 2 shows distance/presence info.
- [ ] Debug page (if enabled) shows useful internal values.
- [ ] Encoder rotation shows temporary offset overlay immediately.
- [ ] Overlay hides after timeout and returns to prior page.
- [ ] Repeated encoder movement extends/resets overlay timeout correctly.

### B3. RGB status behavior
- [ ] Current implementation behavior is documented and verified.
- [ ] Baseline mapping is verified:
- [ ] Blue = AUTO
- [ ] Green = manual offset active (non-zero offset)
- [ ] Yellow = no user (while light ON)
- [ ] Red = manual OFF
- [ ] Purple = boot/setup window
- [ ] Fatal fault = red blink

## C. Acceptance Tests (Pass/Fail)
| Test | Pass Criteria | Result |
|---|---|---|
| Presence + low light | After toggling ON, lamp brightness is appropriate | [ ] |
| Presence + bright light | After toggling ON, lamp stays on but at reduced brightness | [ ] |
| User leaves | Lamp turns off per policy (immediate or timeout) | [ ] |
| Lighting noise | No visible oscillation/flicker | [ ] |
| Toggle control | Single click toggles lamp ON/OFF reliably | [ ] |
| Offset control | Encoder rotation changes brightness smoothly | [ ] |
| Reset control | Double click resets to AUTO baseline brightness | [ ] |

## D. Test Log
| Date | Firmware version/commit | Tester | Notes |
|---|---|---|---|
| | | | |
