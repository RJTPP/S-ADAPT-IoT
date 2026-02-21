# S-ADAPT Test Checklist

## Preconditions
- [x] Firmware builds in `Debug`.
- [x] Board powered with stable `+5V` and `+3.3V`.
- [x] OLED connected and visible.
- [x] HC-SR04, LDR, and LED driver wiring verified against schematic.

## A. Hardware Handling Code Tests

### A1. Sensor and interface bring-up
- [x] OLED initializes and prints boot text.
- [x] Ultrasonic distance updates continuously.
- [x] LDR ADC value changes with lighting condition.
- [x] PWM output present on `TIM1_CH1` pin.

### A2. Module-level robustness
- [x] Ultrasonic timeout path returns safe value when echo is missing.
- [x] Overcapture/noise handling does not lock control loop.
- [x] OLED update loop does not freeze I2C bus.
- [x] Error blink path works if display init is intentionally forced to fail.

### A3. Electrical behavior
- [x] Isolated MOSFET module PWM input control is clean (no chatter at steady duty).
- [x] LED can fully turn off and reach target max output.
- [x] No reset or brownout during fast brightness changes.

## B. Business Logic Tests

### B1. Presence and mode behavior
- [x] OFF->ON captures fallback reference then first valid filtered distance.
- [x] Away rule: `distance > ref + 20` for ~5s in debug-timer profile (`APP_PRESENCE_DEBUG_TIMERS=1`) triggers pre-off dim.
- [x] Flat rule: low step-to-step movement (`abs(step)<=1`) for ~15s in debug-timer profile (`APP_PRESENCE_DEBUG_TIMERS=1`) triggers pre-off dim.
- [x] Pre-off dim behavior: output becomes `min(current,15%)` and stays for configured duration.
- [x] Recovery during pre-off cancels no-user transition.
- [x] Away recovery: return near reference (`ref+10` band) and hold for ~1.5s resumes present.
- [x] Flat recovery: single movement spike (`abs(step)>=2`) resumes present.
- [x] Auto mode computes brightness from LDR.
- [x] Boot state keeps main light OFF until user single-click.
- [x] Encoder single click toggles main light ON/OFF.
- [x] Encoder rotation adjusts manual brightness offset (CW up / CCW down).
- [x] Encoder rotation changes offset only (does not change sensitivity/factor parameters).
- [x] Double click resets manual offset to `0` (AUTO baseline).
- [x] LDR-based AUTO logic remains active after offset reset.
- [x] Presence holds last valid state during transient ultrasonic failures.

### B1.1 Stability filter behavior
- [x] `ldr_filt` responds smoother than `ldr_raw` under small ambient changes.
- [x] `dist_cm_filt` rejects one-sample ultrasonic spikes better than raw.
- [x] Control loop runs at ~33 ms while LDR remains decoupled at ~50 ms and ultrasonic at ~100 ms.
- [x] Output hysteresis deadband works:
- [x] target change `<5%` does not update hysteresis output.
- [x] target change `>=5%` updates hysteresis output.
- [x] Output ramp works:
- [x] normal tracking changes by at most `1%` per `33 ms` tick.
- [x] turn-on transitions use up to `3%` per `33 ms` tick.
- [x] turn-off transitions use up to `5%` per `33 ms` tick.
- [x] Forced-off paths (manual OFF / no-user) ramp down smoothly to `0%`.

### B2. OLED behavior
- [x] Page button cycles pages with wraparound logic.
- [x] Page 0 (`MAIN`) shows mode + LDR% + output% + offset with bars.
- [x] Page 1 (`SENSOR`) shows distance + filtered LDR raw + reference + present/reason flags.
- [x] Bottom-right page bullets are visible on persistent pages and match active page.
- [x] Passive value changes (LDR/output/distance) trigger display updates without waiting only for 1-second refresh.
- [x] OLED redraw stays bounded near 15 FPS max under rapid value changes.
- [x] Encoder rotation shows temporary offset overlay immediately.
- [x] Overlay hides bullets while active.
- [x] Overlay timeout starts at ~1200 ms from last encoder step.
- [x] Overlay remains visible until animated offset reaches target, then holds ~750 ms before returning.
- [x] Repeated encoder movement extends/resets overlay timeout correctly.

### B2.1 Settings mode behavior
- [x] `BUTTON` long-press (~1000 ms) enters settings mode.
- [x] Long-press while in settings mode exits and discards unsaved edits.
- [x] Encoder rotate moves selected settings row in browse mode.
- [x] Encoder click on numeric field enters/leaves edit mode.
- [x] In edit mode, only the value token is inverted.
- [x] Encoder click on binary field toggles ON/OFF immediately in draft.
- [x] `Save` writes settings and applies runtime values.
- [x] `Reset` restores draft to build defaults (not persisted until Save).
- [x] `Exit` discards unsaved draft and returns to normal pages.
- [x] Settings mode suppresses normal page bullets and offset overlay.

### B2.2 Settings persistence behavior
- [x] Save + power cycle restores the same settings values.
- [x] First boot without valid record falls back to defaults cleanly.
- [x] Invalid/corrupt record is ignored and does not break runtime boot.
- [x] Page-full path (append until full) erases and rewrites a fresh valid record.

### B3. RGB status behavior
- [x] Current implementation behavior is documented and verified.
- [x] Baseline mapping is verified:
- [x] Blue = AUTO
- [x] Green = manual offset active (non-zero offset)
- [x] Yellow = no user (while light ON)
- [x] Red = manual OFF
- [x] Purple = boot/setup window
- [x] Fatal fault = red blink

## C. Acceptance Tests (Pass/Fail)
| Test | Pass Criteria | Result |
|---|---|---|
| Presence + low light | After toggling ON, lamp brightness is appropriate | [x] |
| Presence + bright light | After toggling ON, lamp stays on but at reduced brightness | [x] |
| User leaves | Lamp turns off per policy (immediate or timeout) | [x] |
| Lighting noise | No visible oscillation/flicker | [x] |
| Toggle control | Single click toggles lamp ON/OFF reliably | [x] |
| Offset control | Encoder rotation changes brightness smoothly | [x] |
| Reset control | Double click resets to AUTO baseline brightness | [x] |
