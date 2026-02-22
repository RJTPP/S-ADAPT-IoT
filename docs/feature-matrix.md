# S-ADAPT Feature Matrix

## Overview
This matrix summarizes implemented features and current status for the `v1.1.1` release.

## Core Runtime
| Area | Feature | Status |
|---|---|---|
| Runtime ownership | `app_init` / `app_step` orchestrator flow | Implemented |
| Control cadence | 33 ms control tick | Implemented |
| Sensor cadence | 50 ms LDR, 100 ms ultrasonic (decoupled) | Implemented |
| Main output | PWM lamp control (`AUTO + offset`) | Implemented |
| Stability | Hysteresis + ramp limiter | Implemented |

## Inputs and Interaction
| Area | Feature | Status |
|---|---|---|
| Encoder rotate | Manual offset adjust (normal mode) | Implemented |
| Encoder click | Short click toggle / long press offset reset | Implemented |
| Page button | Page switch on short press | Implemented |
| Long press | Settings mode enter/exit (`~1000 ms`) | Implemented |

## Presence Engine
| Area | Feature | Status |
|---|---|---|
| Reference capture | OFF->ON reference acquisition | Implemented |
| Path A | Away detection timeout path | Implemented |
| Path B | Flat/stale detection timeout path | Implemented |
| Pre-off dim | Dimming stage before no-user commit | Implemented |
| Recovery | Away and flat reason-specific recovery rules | Implemented |
| Transient handling | Hold-last-valid on invalid ultrasonic reads | Implemented |

## OLED UI
| Area | Feature | Status |
|---|---|---|
| Main pages | `MAIN` and `SENSOR` pages | Implemented |
| Overlay | Temporary offset overlay + animation/hold | Implemented |
| Settings page | Modal settings list + encoder edit flow | Implemented |
| Edit UX | Value-token invert focus, unsaved marker, exit warning | Implemented |
| Navigation cue | Settings scrollbar | Implemented |
| Refresh | Data/event-driven with ~15 FPS cap | Implemented |

## Persistence
| Area | Feature | Status |
|---|---|---|
| Storage medium | Internal flash reserved page | Implemented |
| Record format | `magic/version/payload_len/seq/payload/crc32` | Implemented |
| Write strategy | Append-only records | Implemented |
| Boot load | Latest valid sequence record restore | Implemented |
| Full-page handling | Erase and restart log when full | Implemented |
| Corruption safety | CRC validation + fallback defaults | Implemented |

## Configurable Settings (Current)
| Setting | Range | Step | Default* |
|---|---:|---:|---:|
| `away_mode_enabled` | `0..1` | toggle | `1` |
| `flat_mode_enabled` | `0..1` | toggle | `1` |
| `away_timeout_s` | `5..120` | `5` | build profile |
| `stale_timeout_s` | `10..300` | `5` | build profile |
| `preoff_dim_s` | `5..30` | `5` | build profile |
| `return_band_cm` | `5..30` | `5` | `10` |

\* Defaults are seeded from current build profile for timing fields (`APP_PRESENCE_DEBUG_TIMERS`), then overridden by saved flash settings when available.
