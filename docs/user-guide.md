# S-ADAPT User Guide

## 1) What This System Does
S-ADAPT automatically adjusts desk-lamp brightness based on:
- ambient light (LDR sensor), and
- user presence (ultrasonic distance).

You can also apply a manual brightness offset with the encoder.

## 2) Controls
- `Encoder rotate`:
  - normal mode: adjust manual offset (`+/-`).
  - settings mode: move selection or change value.
- `Encoder click`:
  - normal mode:
    - short click: light ON/OFF
    - long press (~`800 ms`): reset offset to `0` (fires while still holding)
  - settings mode: enter/exit value edit, or select action.
- `BUTTON` short press:
  - normal mode: switch OLED page.
- `BUTTON` long press (~`1000 ms`):
  - enter/exit settings mode (immediate press-and-hold trigger).

## 3) OLED Pages
### Page 0: MAIN
Shows:
- mode (`ON` / `OFF` / `SLEEP`)
- ambient light percentage (`LDR`)
- output percentage (`OUT`)
- manual offset value (`OFF:+/-n`)
- progress bars for `LDR` and `OUT`

### Page 1: SENSOR
Shows:
- distance (`DIST`)
- filtered LDR raw (`LDRF`)
- reference distance (`REF`)
- presence and reason flags (`PRS`, `RSN`)

### Offset Overlay
When rotating encoder in normal mode, a temporary centered offset overlay appears.

## 4) Settings Mode
Enter with `BUTTON` long press.

Settings rows:
1. `Away Mode` (ON/OFF)
2. `Flat Mode` (ON/OFF)
3. `Away T` (seconds)
4. `Flat T` (seconds)
5. `PreOff` (seconds)
6. `RetBand` (cm)
7. `Save`
8. `Reset`
9. `Exit`

Behavior:
- Rotate in browse mode: move selected row.
- Click numeric row: enter value edit mode.
- Rotate in edit mode: change value.
- Click again: leave value edit mode.
- Click binary row: toggle ON/OFF.
- Click `Save`: persist to flash and apply runtime.
- Click `Reset`: load default values into draft.
- Click `Exit`: leave settings (unsaved draft is discarded).

Visual cues:
- `>` = selected row
- `E` = editing mode
- `*` = unsaved draft changes
- Value token invert = currently focused editable value
- `Exit?` inverted = warning cue when unsaved changes exist
- Right-side scrollbar = position in settings list

## 5) Presence Behavior (Practical)
S-ADAPT uses two no-user detection paths:
- Path A (Away): user moved farther than reference + margin for timeout.
- Path B (Flat/Stale): distance change stays too flat for timeout.

Before committing no-user off, system enters a pre-off dim phase.
If user returns during pre-off, off transition is canceled.

## 6) Persistence
Saved settings are stored in internal flash and restored on reboot.
- `Save` writes a new record.
- `Exit` without save does not persist draft edits.

## 7) Abbreviation / Short Word Meanings
### OLED (Main/Sensor pages)
- `MODE` = operating state (`ON`, `OFF`, `SLEEP`)
- `LDR` = Light Dependent Resistor input as percentage
- `OUT` = final applied lamp output percentage
- `OFF:+n/-n` = manual **offset** (not power-off)
- `DIST` = measured distance (cm)
- `LDRF` = filtered LDR raw ADC value
- `REF` = reference distance used by presence logic
- `PRS` = present flag (`1` present, `0` not present)
- `RSN` = no-user reason code
- `NON` = none
- `AWY` = away
- `FLT` = flat/stale

### Status badges / indicators
- `LEAVE` = leaving/no-user transition indicator
- `DIM` = pre-off dim phase active
- `AWAY` = away-related status
- `IDLE` = idle-related status

### Settings labels
- `Away T` = away timeout (seconds)
- `Flat T` = flat/stale timeout (seconds)
- `PreOff` = pre-off dim duration (seconds)
- `RetBand` = return band threshold for away recovery, relative to reference distance (cm)

### Runtime logs (common)
- `ldr_raw` = raw LDR ADC
- `ldr_filt` = filtered LDR ADC
- `dist_cm_filt` = filtered distance in cm
- `target_out` = control target output before hysteresis/ramp
- `hyst_out` = output after hysteresis
- `applied_out` = final output sent to PWM
- `cfg_*` = active saved configuration values

## 8) Quick Troubleshooting
- Cannot enter settings:
  - hold `BUTTON` for about `1000 ms`.
- Value changes but not kept after reboot:
  - ensure `Save` was selected.
- Exited settings and changes disappeared:
  - expected if not saved (`Exit` discards draft).
