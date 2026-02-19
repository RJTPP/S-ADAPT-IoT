# S-ADAPT Calibration Guide

## Goal
Tune thresholds and filters so behavior is stable, responsive, and comfortable for users.

## Parameters to Tune
| Parameter | Purpose | Starting Value | Typical Range |
|---|---|---|---|
| Presence threshold (cm) | Decide user present/absent | 80 | 50-120 |
| No-presence timeout (s) | Delay before standby/off | 30 | 5-60 |
| LDR filter window | Smooth ambient noise | 8 samples | 4-32 |
| Ultrasonic median window | Reject spikes | 5 samples | 3-9 |
| Brightness min (%) | Prevent too-dim output | 20 | 0-40 |
| Brightness max (%) | Cap output power | 100 | 70-100 |
| Brightness hysteresis (%) | Avoid flicker | 3 | 1-10 |
| Brightness step per tick (manual) | Encoder feel | 2 | 1-10 |

## Recommended Procedure
1. Verify raw sensor reads are sane:
1. LDR changes when light changes.
1. Distance tracks object movement without random spikes.
1. Tune presence threshold:
1. Measure typical seated distance.
1. Set threshold with margin to avoid accidental off events.
1. Tune LDR mapping:
1. Capture values in bright, normal, and dim rooms.
1. Set map endpoints so brightness feels natural.
1. Tune stability:
1. Increase filter window if output flickers.
1. Increase hysteresis if PWM oscillates near boundary.
1. Tune manual mode:
1. Adjust encoder step size for usable control speed.

## Data Collection Template
Use this table during tuning:

| Scenario | Distance cm | LDR raw | LDR filtered | Brightness % | Mode | Notes |
|---|---:|---:|---:|---:|---|---|
| Bright room, user present | | | | | | |
| Dim room, user present | | | | | | |
| User leaves desk | | | | | | |
| Noise/stress case | | | | | | |

## Common Symptoms and Fixes
| Symptom | Likely Cause | Fix |
|---|---|---|
| Rapid brightness flicker | Filter/hysteresis too low | Increase LDR filter and hysteresis |
| False no-user events | Threshold too small or noisy echo | Raise threshold, improve ultrasonic filter |
| Sluggish response | Filter window too large | Reduce window size |
| OLED laggy updates | Too much rendering work per tick | Reduce update frequency or page complexity |

## Completion Criteria
- Presence detection is stable for normal seating distance.
- Brightness changes are smooth and not jumpy.
- Manual mode feels predictable.
- Standby/off transitions are intentional, not accidental.
